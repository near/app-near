#![no_std]
use borsh::{
    io::{Read, Result},
    BorshDeserialize,
};
use fmt_buffer::Buffer;

/// Balance is type for storing amounts of tokens.
type Balance = u128;

const ONE_NEAR: u128 = 10_u128.pow(24);
const ONE_MILLINEAR: u128 = 10_u128.pow(21);
const ONE_MICRONEAR: u128 = 10_u128.pow(18);
use numtoa::NumToA;

/// A buffer, large enough to fit all [NearToken] representation values into
pub type TokenBuffer = Buffer<30>;

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
pub struct NearToken(pub Balance);

impl NearToken {
    /// `from_yoctonear` is a function that takes value by a number of yocto-near.
    /// # Examples
    /// ```
    /// use near_token::NearToken;
    /// assert_eq!( NearToken::from_yoctonear(10u128.pow(21)), NearToken::from_millinear(1))
    /// ```
    pub const fn from_yoctonear(inner: u128) -> Self {
        Self(inner)
    }

    /// `from_millinear` is a function that takes value by a number of mili-near and converts it to an equivalent to the yocto-near.
    /// # Examples
    /// ```
    /// use near_token::NearToken;
    /// assert_eq!(NearToken::from_millinear(1), NearToken::from_yoctonear(10u128.pow(21)))
    /// ```
    pub const fn from_near(inner: u128) -> Self {
        Self(inner * ONE_NEAR)
    }

    pub const fn from_millinear(inner: u128) -> Self {
        Self(inner * ONE_MILLINEAR)
    }

    /// `from_micronear` is a function that takes value by a number of micro-near and converts it to an equivalent to the yocto-near.
    /// # Examples
    /// ```
    /// use near_token::NearToken;
    /// assert_eq!(NearToken::from_micronear(1), NearToken::from_yoctonear(10u128.pow(18)))
    /// ```
    pub const fn from_micronear(inner: u128) -> Self {
        Self(inner * ONE_MICRONEAR)
    }

    /// `as_yoctonear` is a function that shows a number of yocto-near.
    /// # Examples
    /// ```
    /// use near_token::NearToken;
    /// assert_eq!(NearToken::from_yoctonear(10).as_yoctonear(), 10)
    /// ```
    pub const fn as_yoctonear(&self) -> u128 {
        self.0
    }

    pub fn display_as_buffer(&self, result: &mut TokenBuffer) {
        if *self == NearToken::from_yoctonear(0) {
            result.write_str("0 NEAR");
        } else if *self == NearToken::from_yoctonear(1) {
            result.write_str("1 yoctoNEAR");
        } else if *self < NearToken::from_micronear(1) {
            result.write_str("less than 0.000001 NEAR");
        } else if *self < NearToken::from_millinear(1) {
            result.write_str("0.");
            let mut str_buf = [0u8; 10];
            let micronear_rounded_up =
                (self.as_yoctonear().saturating_add(ONE_MICRONEAR - 1) / ONE_MICRONEAR) as u32;

            let micro_str = micronear_rounded_up.numtoa_str(10, &mut str_buf);
            let leading_zeros = 6 - micro_str.len();
            for _ in 0..leading_zeros {
                result.write_str("0");
            }

            // Remove trailing zeros from micro_str before writing to buffer
            let trimmed_micro_str = micro_str.trim_end_matches('0');
            result.write_str(trimmed_micro_str);

            result.write_str(" NEAR");
        } else if *self <= NearToken::from_millinear(999) {
            let millinear_rounded_up =
                (self.as_yoctonear().saturating_add(ONE_MILLINEAR - 1) / ONE_MILLINEAR) as u32;

            let mut millis_str_buf = [0u8; 10];

            result.write_str("0.");
            // numtoa_buf has to be at least 10 bytes for u32 (4 bytes) : ok
            let millis_str = millinear_rounded_up.numtoa_str(10, &mut millis_str_buf);
            let leading_zeros = 3 - millis_str.len();
            for _ in 0..leading_zeros {
                result.write_str("0");
            }

            // Remove trailing zeros from millis_str before writing to buffer
            let trimmed_millis_str = millis_str.trim_end_matches('0');

            result.write_str(trimmed_millis_str);
            result.write_str(" NEAR");
        } else {
            let near_rounded_up =
                self.as_yoctonear().saturating_add(10 * ONE_MILLINEAR - 1) / ONE_MILLINEAR / 10;
            let mut str_buf = [0u8; 20];

            // numtoa_buf has to be at least 20 bytes for u64 (8 bytes) : ok
            result.write_str((near_rounded_up as u64 / 100).numtoa_str(10, &mut str_buf));
            result.write_str(".");
            // numtoa_buf has to be at least 20 bytes for u64 (8 bytes) : ok
            let hundreds_str = (near_rounded_up as u64 % 100).numtoa_str(10, &mut str_buf);
            let leading_zeros = 2 - hundreds_str.len();
            for _ in 0..leading_zeros {
                result.write_str("0");
            }
            result.write_str(hundreds_str);
            result.write_str(" NEAR");
        }
    }
}

impl BorshDeserialize for NearToken {
    fn deserialize_reader<R: Read>(reader: &mut R) -> Result<Self> {
        let inner: Balance = BorshDeserialize::deserialize_reader(reader)?;

        Ok(Self::from_yoctonear(inner))
    }
}
#[cfg(test)]
mod tests {
    use crate::TokenBuffer;

    use super::NearToken;

    #[test]
    fn test_display() {
        for (integer, expected) in [
            (1234560000000000000000000000u128, "1234.56 NEAR"),
            (10000000000000000000, "0.00001 NEAR"),
            (10000000000000000000000, "0.01 NEAR"),
            (0, "0 NEAR"),
            (1, "1 yoctoNEAR"),
        ] {
            let mut buffer: TokenBuffer = TokenBuffer::new();

            let token = NearToken::from_yoctonear(integer);

            token.display_as_buffer(&mut buffer);

            assert_eq!(buffer.as_str(), expected);
            assert_eq!(buffer.truncated(), false);
        }

        for (integer_millis, expected) in [
            (1, "0.001 NEAR"),
            (11, "0.011 NEAR"),
            (111, "0.111 NEAR"),
            (1000, "1.00 NEAR"),
            (1001, "1.01 NEAR"),
            (1010, "1.01 NEAR"),
            (1100, "1.10 NEAR"),
            (1000 * 1_000_000, "1000000.00 NEAR"),
        ] {
            let mut buffer: TokenBuffer = TokenBuffer::new();

            let token = NearToken::from_millinear(integer_millis);

            token.display_as_buffer(&mut buffer);

            assert_eq!(buffer.as_str(), expected);
            assert_eq!(buffer.truncated(), false);
        }
    }
}
