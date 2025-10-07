use crate::utils::crypto::{PathBip32, PublicKeyBe};
use fmt_buffer::Buffer;
use near_token::{NearToken, TokenBuffer};

use ledger_device_sdk::{
    ecc,
    io::Comm,
    libcall::{
        self,
        swap::{self, CheckAddressParams, CreateTxParams, PrintableAmountParams},
    },
    testing::debug_print,
};

use crate::parsing::transaction_stream_reader::SingleTxStream;

pub fn swap_main(arg0: u32) {
    debug_print("call app for swap \n");

    let cmd = libcall::get_command(arg0);

    match cmd {
        libcall::LibCallCommand::SwapCheckAddress => {
            let mut params = swap::get_check_address_params(arg0);
            let res = match check_address(&params) {
                Ok(_) => 1,
                Err(err) => {
                    debug_print(err);
                    0
                }
            };
            swap::swap_return(swap::SwapResult::CheckAddressResult(&mut params, res));
        }
        libcall::LibCallCommand::SwapGetPrintableAmount => {
            let mut params = swap::get_printable_amount_params(arg0);
            let mut s = get_printable_amount(&params);
            swap::swap_return(swap::SwapResult::PrintableAmountResult(
                &mut params,
                s.as_str(),
            ));
        }
        libcall::LibCallCommand::SwapSignTransaction => {
            let mut params = swap::sign_tx_params(arg0);

            {
                let mut comm = Comm::new().set_expected_cla(super::CLA);

                debug_print("Wait for APDU\n");

                loop {
                    // Wait for an APDU command
                    let ins: super::Instruction = comm.next_command();

                    debug_print("APDU received\n");

                    swap_handle_apdu(&mut comm, ins, &mut params);
                }
            }
        }
    }
}

fn swap_handle_apdu(comm: &mut Comm, ins: super::Instruction, tx_params: &mut CreateTxParams) {
    match ins {
        super::Instruction::SignTx {
            is_last_chunk,
            sign_mode,
        } => {
            debug_print("handle_swap_apdu => Sign Tx\n");
            let stream = SingleTxStream::new(comm, is_last_chunk, sign_mode);
            match sign_mode {
                super::SignMode::Transaction => {
                    let signature = crate::handlers::sign_tx::swap_handler(stream, tx_params);
                    match signature {
                        Ok(sig) => {
                            comm.append(&sig.0);
                            comm.swap_reply_ok();
                            swap::swap_return(swap::SwapResult::CreateTxResult(tx_params, 1));
                        }
                        Err(sw) => {
                            comm.swap_reply(sw);
                            swap::swap_return(swap::SwapResult::CreateTxResult(tx_params, 0));
                        }
                    }
                }
                _ => {
                    comm.swap_reply(crate::AppSW::TxSignFail);
                    swap::swap_return(swap::SwapResult::CreateTxResult(tx_params, 0));
                }
            }
        }
        super::Instruction::GetPubkey { display } => match display {
            true => comm.swap_reply(crate::AppSW::InsNotSupported),
            false => match crate::handlers::get_public_key::handler(comm, display) {
                Ok(()) => comm.swap_reply_ok(),
                Err(sw) => comm.swap_reply(sw),
            },
        },
        _ => comm.swap_reply(crate::AppSW::InsNotSupported),
    }
}

fn check_address(params: &CheckAddressParams) -> Result<(), &'static str> {
    let path = PathBip32::parse(&params.dpath[..params.dpath_len * 4])
        .map_err(|_| "Derivation path failure")?;

    let pk = ecc::Ed25519::derive_from_path_slip10(&path.0)
        .public_key()
        .map_err(|_| "Public key derivation failure")?;

    let pk = PublicKeyBe::from_little_endian(pk);
    let mut buf = [0u8; 64];
    let address = pk.display_str_hex(&mut buf);

    let ref_address = core::str::from_utf8(&params.ref_address[..params.ref_address_len])
        .map_err(|_| "Invalid UTF-8 in reference address")?;

    if address == ref_address {
        Ok(())
    } else {
        Err("Address mismatch")
    }
}

fn get_printable_amount(params: &PrintableAmountParams) -> Buffer<30> {
    match params.is_fee {
        true => {
            let amount = u128::from_be_bytes(params.amount);
            let gas = NearToken::from_yoctonear(amount);
            let mut near_gas_buffer = TokenBuffer::new();

            gas.display_as_buffer(&mut near_gas_buffer);
            near_gas_buffer
        }
        false => {
            let amount = u128::from_be_bytes(params.amount);
            let near_token = NearToken::from_yoctonear(amount);
            let mut near_token_buffer = TokenBuffer::new();
            near_token.display_as_buffer(&mut near_token_buffer);
            near_token_buffer
        }
    }
}
