use borsh::io::{ErrorKind, Read, Result};
use borsh::BorshDeserialize;

pub enum KeyType {
    ED25519 = 0,
    SECP256K1 = 1,
}

#[derive(Clone, Copy)]
pub enum TxPublicKey {
    /// 256 bit elliptic curve based public-key.
    ED25519([u8; 32]),
    /// 512 bit elliptic curve based public-key used in Bitcoin's public-key cryptography.
    SECP256K1([u8; 64]),
}

impl TryFrom<u8> for KeyType {
    type Error = borsh::io::Error;

    fn try_from(value: u8) -> core::result::Result<Self, Self::Error> {
        match value {
            0 => Ok(KeyType::ED25519),
            1 => Ok(KeyType::SECP256K1),
            _unknown_key_type => Err(Self::Error::from(ErrorKind::InvalidData)),
        }
    }
}

impl BorshDeserialize for TxPublicKey {
    fn deserialize_reader<R: Read>(rd: &mut R) -> Result<Self> {
        let key_type = KeyType::try_from(u8::deserialize_reader(rd)?)?;
        match key_type {
            KeyType::ED25519 => Ok(TxPublicKey::ED25519(BorshDeserialize::deserialize_reader(
                rd,
            )?)),
            KeyType::SECP256K1 => Ok(TxPublicKey::SECP256K1(
                BorshDeserialize::deserialize_reader(rd)?,
            )),
        }
    }
}
