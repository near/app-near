use crate::app_ui::aliases::{FnCallCappedString, FnCallHexDisplay};
use crate::parsing::types::FunctionCallCommon;
use crate::sign_ui;
use crate::utils::types::capped_string::CappedString;
use crate::{
    parsing::{HashingStream, SingleTxStream},
    AppSW,
};
use borsh::io::ErrorKind;
use borsh::BorshDeserialize;

use super::ActionParams;

pub fn handle(
    stream: &mut HashingStream<SingleTxStream<'_>>,
    params: ActionParams,
) -> Result<(), AppSW> {
    let mut method_name: CappedString<50> = CappedString::new();

    method_name
        .deserialize_reader_in_place(stream)
        .map_err(|_err| AppSW::TxParsingFail)?;

    handle_from_method_name(stream, method_name, params)
}

/// Continue FunctionCall handling after the method_name has already been consumed
/// from the stream. Used by sign_tx.rs when the method name was pre-read for
/// staking-method detection.
pub fn handle_from_method_name(
    stream: &mut HashingStream<SingleTxStream<'_>>,
    method_name: CappedString<50>,
    params: ActionParams,
) -> Result<(), AppSW> {
    let args_bytes_count: u32 =
        u32::deserialize_reader(stream).map_err(|_err| AppSW::TxParsingFail)?;

    let mut representation = match stream
        .reader
        .peek_u8()
        .map_err(|_err| AppSW::TxParsingFail)?
    {
        // '{' char
        Some(123) => {
            let mut args_str: FnCallCappedString = FnCallCappedString::new();
            match args_str.deserialize_with_bytes_count(stream, args_bytes_count) {
                Err(err) if err.kind() == ErrorKind::InvalidData => {
                    let mut args_bin: FnCallHexDisplay = args_str.into();
                    args_bin.reformat();
                    ArgsRepr::BinHex(args_bin)
                }
                Ok(_) => ArgsRepr::String(args_str),
                Err(_err) => {
                    return Err(AppSW::TxParsingFail);
                }
            }
        }
        Some(_first_byte) => {
            let mut args_bin: FnCallHexDisplay = FnCallHexDisplay::new();
            args_bin
                .deserialize_with_bytes_count(stream, args_bytes_count)
                .map_err(|_err| AppSW::TxParsingFail)?;
            args_bin.reformat();

            ArgsRepr::BinHex(args_bin)
        }
        None => {
            return Err(AppSW::TxParsingFail);
        }
    };
    handle_common(stream, method_name, params, &mut representation)
}
fn handle_common(
    stream: &mut HashingStream<SingleTxStream<'_>>,
    method_name: CappedString<50>,
    params: ActionParams,
    representation: &mut ArgsRepr,
) -> Result<(), AppSW> {
    let mut func_call_common =
        FunctionCallCommon::deserialize_with_method_name(stream, method_name)
            .map_err(|_err| AppSW::TxParsingFail)?;
    match representation {
        ArgsRepr::BinHex(args_bin) => {
            if !sign_ui::action::ui_display_function_call_bin(
                &mut func_call_common,
                args_bin,
                params,
            ) {
                return Err(AppSW::Deny);
            }
        }
        ArgsRepr::String(args_str) => {
            if !sign_ui::action::ui_display_function_call_str(
                &mut func_call_common,
                args_str,
                params,
            ) {
                return Err(AppSW::Deny);
            }
        }
    }
    Ok(())
}

enum ArgsRepr {
    String(FnCallCappedString),
    BinHex(FnCallHexDisplay),
}
