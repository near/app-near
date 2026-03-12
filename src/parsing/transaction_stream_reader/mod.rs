#[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
use ledger_device_sdk::buttons::ButtonEvent;
use ledger_device_sdk::io::{Comm, Event};

use ledger_device_sdk::hash::sha2::Sha2_256;
use ledger_device_sdk::hash::HashInit;

use crate::{AppSW, Instruction, SignMode};
use borsh::io::{self};

pub struct SingleTxStream<'a> {
    pub comm: &'a mut Comm,
    chunk_counter: usize,
    is_last_chunk: bool,
    sign_mode: SignMode,
}

impl<'a> SingleTxStream<'a> {
    pub fn new(comm: &'a mut Comm, is_last_chunk: bool, sign_mode: SignMode) -> Self {
        let total_counter = 0;
        Self {
            comm,
            chunk_counter: total_counter,
            is_last_chunk,
            sign_mode,
        }
    }
}

pub struct Sha256Digest(pub [u8; 32]);

pub struct HashingStream<R> {
    pub reader: R,
    sha256: Sha2_256,
}

impl<R> HashingStream<R> {
    pub fn new(reader: R) -> Result<Self, AppSW> {
        let sha256 = Sha2_256::new();
        let res = Self { reader, sha256 };
        Ok(res)
    }

    pub fn finalize(mut self) -> Result<Sha256Digest, AppSW> {
        let mut array = [0u8; 32];

        self.sha256
            .finalize(&mut array)
            .map_err(|_err| AppSW::TxHashFinalizeFail)?;
        Ok(Sha256Digest(array))
    }
}
impl<R> HashingStream<R> {
    pub fn feed_slice(&mut self, input: &[u8]) -> io::Result<()> {
        self.sha256
            .update(input)
            .map_err(|_err| io::Error::from(io::ErrorKind::OutOfMemory))
    }
}

impl<R: io::Read> io::Read for HashingStream<R> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        if !buf.is_empty() {
            let n = self.reader.read(buf)?;

            // update hash on each chunk passing through
            if n > 0 {
                let data = &buf[0..n];
                self.sha256
                    .update(data)
                    .map_err(|_err| io::Error::from(io::ErrorKind::OutOfMemory))?;
            }
            return Ok(n);
        }
        Ok(0)
    }
}

impl SingleTxStream<'_> {
    pub fn remaining_in_current_chunk(&mut self) -> io::Result<&[u8]> {
        let data = self
            .comm
            .get_data()
            .map_err(|_err| io::Error::from(io::ErrorKind::BrokenPipe))?;

        let (_read, left) = data.split_at(self.chunk_counter);
        Ok(left)
    }

    pub fn peek_u8(&mut self) -> io::Result<Option<u8>> {
        let data = self
            .comm
            .get_data()
            .map_err(|_err| io::Error::from(io::ErrorKind::BrokenPipe))?;

        let (_read, left) = data.split_at(self.chunk_counter);

        if left.is_empty() && self.is_last_chunk {
            return Ok(None);
        }

        if !left.is_empty() {
            return Ok(Some(left[0]));
        }

        let data = self.get_next_chunk()?;
        Ok(Some(data[0]))
    }

    fn get_next_chunk(&mut self) -> io::Result<&[u8]> {
        let is_last_chunk = loop {
            match self.comm.next_event() {
                #[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
                Event::Button(ButtonEvent::BothButtonsRelease) => {
                    return Err(io::Error::from(io::ErrorKind::Interrupted))
                }
                Event::Command(Instruction::GetVersion)
                | Event::Command(Instruction::GetPubkey { .. }) => {
                    return Err(io::Error::from(io::ErrorKind::InvalidData))
                }
                Event::Command(Instruction::SignTx {
                    is_last_chunk,
                    sign_mode,
                }) if sign_mode == self.sign_mode => break is_last_chunk,
                Event::Command(Instruction::SignTx { .. }) => {
                    return Err(io::Error::from(io::ErrorKind::InvalidData))
                }
                _ => (),
            };
        };

        self.is_last_chunk = is_last_chunk;
        self.chunk_counter = 0;
        let data = self
            .comm
            .get_data()
            .map_err(|_err| io::Error::from(io::ErrorKind::BrokenPipe))?;
        if data.is_empty() {
            return Err(io::Error::from(io::ErrorKind::InvalidData));
        }
        Ok(data)
    }
}

impl io::Read for SingleTxStream<'_> {
    fn read(&mut self, buf: &mut [u8]) -> io::Result<usize> {
        let data = self
            .comm
            .get_data()
            .map_err(|_err| io::Error::from(io::ErrorKind::BrokenPipe))?;

        let (_read, mut left) = data.split_at(self.chunk_counter);

        if left.is_empty() && self.is_last_chunk {
            return Ok(0);
        }

        if !left.is_empty() {
            let n = left.read(buf)?;
            self.chunk_counter += n;
            return Ok(n);
        }

        // first inform the sender we're ready to receive next chunk
        self.comm.reply_ok();
        let mut data = self.get_next_chunk()?;
        let n = data.read(buf)?;
        self.chunk_counter += n;
        Ok(n)
    }
}
