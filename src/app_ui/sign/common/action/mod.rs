use crate::app_ui::aliases::{FnCallCappedString, FnCallHexDisplay, U32Buffer};
use crate::{app_ui::fields_writer::FieldsWriter, handlers::common::action::ActionParams, parsing};
use fmt_buffer::Buffer;

#[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
use crate::Instruction;
use ledger_device_sdk::io::Comm;
#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use ledger_device_sdk::nbgl::{
    CenteredInfo, CenteredInfoStyle, InfoButton, InfoLongPress, NbglChoice, NbglGenericReview,
    NbglPageContent, NbglStatus, TagValueList, TuneIndex,
};
#[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
use ledger_device_sdk::{
    buttons::ButtonEvent,
    io::Event,
    ui::{
        bitmaps::{CROSSMARK, EYE, VALIDATE_14, WARNING},
        gadgets::{clear_screen, MultiFieldReview},
        layout::{Layout, Location, StringPlace},
        screen_util::screen_update,
    },
};

#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use crate::app_ui::logo::NEAR_LOGO;
use numtoa::NumToA;

use super::tx_public_key_context;

#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use core::ops::Not;

use ledger_device_sdk::log;

mod add_key_common;
mod create_account;
mod delete_account;
mod delete_key;
mod deploy_contract;
mod function_call_bin;
mod function_call_common;
mod function_call_permission;
mod function_call_str;
mod stake;
pub mod stake_fn_call;
mod transfer;

#[derive(serde::Deserialize)]
struct StringArgs<'a> {
    amount: &'a str,
}

pub fn ui_display_transfer(transfer: &parsing::types::Transfer, params: ActionParams) -> bool {
    let mut field_context: transfer::FieldsContext = transfer::FieldsContext::new();
    let mut writer = FieldsWriter::new();

    transfer::format(transfer, &mut field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_create_account(
    create_account: &parsing::types::CreateAccount,
    params: ActionParams,
) -> bool {
    let mut writer = FieldsWriter::new();

    create_account::format(create_account, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_delete_account(
    delete_account: &mut parsing::types::DeleteAccount,
    params: ActionParams,
) -> bool {
    let mut writer = FieldsWriter::new();
    let mut field_context: delete_account::FieldsContext = delete_account::FieldsContext::new();

    delete_account::format(delete_account, &mut field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_delete_key(delete_key: &parsing::types::DeleteKey, params: ActionParams) -> bool {
    let mut field_context: tx_public_key_context::FieldsContext =
        tx_public_key_context::FieldsContext::new();
    let mut writer = FieldsWriter::new();

    delete_key::format(delete_key, &mut field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_stake(stake: &parsing::types::Stake, params: ActionParams) -> bool {
    let mut field_context: stake::FieldsContext = stake::FieldsContext::new();
    let mut writer = FieldsWriter::new();

    stake::format(stake, &mut field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

/// action type (1) + Public Key (1) + Access Key Nonce (1) +
/// Access Permission (1)
const ADD_KEY_FULL_ACCESS_MAX_FIELDS: usize = 4;

pub fn ui_display_add_key_fullaccess(
    add_key: &parsing::types::AddKey,
    params: ActionParams,
) -> bool {
    let mut field_context: add_key_common::FieldsContext = add_key_common::FieldsContext::new();
    let mut writer: FieldsWriter<'_, ADD_KEY_FULL_ACCESS_MAX_FIELDS> = FieldsWriter::new();

    add_key_common::format(add_key, &mut field_context, &mut writer, "Full Access");

    ui_display_common(&mut writer, params)
}

pub fn ui_display_add_key_functioncall(
    add_key: &parsing::types::AddKey,
    function_call_per: &mut parsing::types::FunctionCallPermission,
    params: ActionParams,
) -> bool {
    let mut common_field_context: add_key_common::FieldsContext =
        add_key_common::FieldsContext::new();
    let mut func_call_field_context: function_call_permission::FieldsContext =
        function_call_permission::FieldsContext::new();
    let mut writer = FieldsWriter::new();

    add_key_common::format(
        add_key,
        &mut common_field_context,
        &mut writer,
        "Function Call",
    );
    function_call_permission::format(function_call_per, &mut func_call_field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_deploy_contract(
    deploy_contract: &parsing::types::DeployContract,
    params: ActionParams,
) -> bool {
    let mut writer = FieldsWriter::new();

    deploy_contract::format(deploy_contract, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_function_call_str(
    func_call_common: &mut parsing::types::FunctionCallCommon,
    args: &mut FnCallCappedString,
    params: ActionParams,
) -> bool {
    let mut writer = FieldsWriter::new();
    let mut common_field_context: function_call_common::FieldsContext =
        function_call_common::FieldsContext::new();

    function_call_common::format(func_call_common, &mut common_field_context, &mut writer);
    let mut args_field_context: function_call_str::FieldsContext =
        function_call_str::FieldsContext::new();
    function_call_str::format(args, &mut args_field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_function_call_bin(
    func_call_common: &mut parsing::types::FunctionCallCommon,
    args: &FnCallHexDisplay,
    params: ActionParams,
) -> bool {
    let mut writer = FieldsWriter::new();
    let mut common_field_context: function_call_common::FieldsContext =
        function_call_common::FieldsContext::new();

    function_call_common::format(func_call_common, &mut common_field_context, &mut writer);
    let mut args_field_context: function_call_bin::FieldsContext =
        function_call_bin::FieldsContext::new();
    function_call_bin::format(args, &mut args_field_context, &mut writer);

    ui_display_common(&mut writer, params)
}

pub fn ui_display_delegate_error(#[allow(unused)] comm: &mut Comm) {
    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    {
        clear_screen();

        // Add icon and text to match the C SDK equivalent.
        WARNING.draw(57, 10);
        "Sign delegate action".place(Location::Custom(28), Layout::Centered, true);
        "not supported...".place(Location::Custom(42), Layout::Centered, true);

        screen_update();
        loop {
            {
                if let Event::Button(ButtonEvent::BothButtonsRelease) =
                    comm.next_event::<Instruction>()
                {
                    return;
                };
            }
        }
    }
    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    {
        let info_button = InfoButton::new(
            "Delegate action is not supported",
            Some(&NEAR_LOGO),
            "Reject",
            TuneIndex::Success,
        );

        let review: NbglGenericReview =
            NbglGenericReview::new().add_content(NbglPageContent::InfoButton(info_button));

        let res = review.show("Reject");

        NbglStatus::new().text("Transaction rejected").show(res);
    }
}

/// Returns `true` if `method_name` is a known staking pool method.
pub fn is_staking_method(method_name: &str) -> bool {
    matches!(
        method_name,
        "deposit_and_stake" | "stake" | "unstake" | "unstake_all" | "withdraw" | "withdraw_all"
    )
}

/// Attempt to parse a yoctoNEAR amount from a JSON args string.
/// Supports:
/// - integer yocto strings: `{"amount":"1157130000000000000000000"}`
/// - decimal NEAR strings: `{"amount":"0.180623655669747439464495"}`
///
/// Returns `None` if the key is absent or unparsable.
///
/// Only call this after a successful `deserialize_with_bytes_count` (i.e. when
/// the args are known to be valid UTF-8).
pub fn try_parse_amount_from_args(args: &mut FnCallCappedString) -> Option<near_token::NearToken> {
    let s = args.as_str();
    let (data, _remainer) = serde_json_core::from_str::<StringArgs<'_>>(s).ok()?;
    log::debug!("amount str: {}", data.amount);
    let yocto = data
        .amount
        .parse::<u128>()
        .ok()
        .or_else(|| parse_decimal_near_to_yocto(data.amount))?;
    log::debug!("amount yocto u128: {}", yocto);
    Some(near_token::NearToken::from_yoctonear(yocto))
}

fn parse_decimal_near_to_yocto(amount: &str) -> Option<u128> {
    let dot_idx = amount.find('.')?;
    if amount[dot_idx + 1..].contains('.') {
        return None;
    }

    let whole = &amount[..dot_idx];
    let frac = &amount[dot_idx + 1..];
    // Reject trailing-dot forms like "1." to keep parsing strict and predictable.
    if whole.is_empty() || frac.is_empty() {
        return None;
    }
    if frac.len() > 24 {
        return None;
    }
    if !whole.as_bytes().iter().all(u8::is_ascii_digit) {
        return None;
    }
    if !frac.as_bytes().iter().all(u8::is_ascii_digit) {
        return None;
    }

    let yocto_per_near = near_token::NearToken::from_near(1).as_yoctonear();
    let whole_part = whole.parse::<u128>().ok()?.checked_mul(yocto_per_near)?;
    let mut frac_buf = [b'0'; 24];
    let frac_len = frac.len();
    frac_buf[..frac_len].copy_from_slice(frac.as_bytes());
    let frac_part = core::str::from_utf8(&frac_buf).ok()?.parse::<u128>().ok()?;

    whole_part.checked_add(frac_part)
}

#[derive(Copy, Clone)]
enum StakeAmountSource {
    Deposit,
    Args,
    NotApplicable,
}

fn stake_method_routing(method_name: &str) -> (stake_fn_call::StakeOpKind, StakeAmountSource) {
    use stake_fn_call::StakeOpKind;

    match method_name {
        "deposit_and_stake" => (StakeOpKind::DepositAndStake, StakeAmountSource::Deposit),
        "stake" => (StakeOpKind::DepositAndStake, StakeAmountSource::Args),
        "unstake" | "withdraw" => (StakeOpKind::Unstake, StakeAmountSource::Args),
        _ => (StakeOpKind::UnstakeAll, StakeAmountSource::NotApplicable),
    }
}

/// Merged prefix + native Stake action review (single combined screen).
pub fn ui_display_stake_combined(
    prefix: &mut parsing::types::transaction::prefix::Prefix,
    stake: &parsing::types::Stake,
) -> bool {
    let mut field_context = stake::CombinedFieldsContext::new();
    let mut writer: FieldsWriter<'_, { stake::COMBINED_MAX_FIELDS }> = FieldsWriter::new();

    stake::format_combined(
        &mut prefix.signer_id,
        stake,
        &mut field_context,
        &mut writer,
    );

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    let review_title = "Review transaction to deposit and stake NEAR";

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    let sign_title = "Sign transaction to deposit and stake NEAR?";

    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    let review_title = "Review stake";

    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    let sign_title = "Sign stake?";

    ui_display_last_action(review_title, sign_title, &mut writer)
}

/// Merged prefix + staking FunctionCall review (single combined screen).
pub fn ui_display_fn_call_stake(
    prefix: &mut parsing::types::transaction::prefix::Prefix,
    method_name: &str,
    amount_opt: Option<near_token::NearToken>,
    gas: near_gas::NearGas,
    deposit: near_token::NearToken,
) -> bool {
    let (kind, amount_source) = stake_method_routing(method_name);

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    let review_title = match method_name {
        "deposit_and_stake" | "stake" => "Review transaction to deposit and stake NEAR",
        "unstake" | "withdraw" | "unstake_all" | "withdraw_all" => {
            "Review transaction to unstake NEAR"
        }
        _ => "Review transaction",
    };

    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    let review_title = match method_name {
        "deposit_and_stake" | "stake" => "Review stake",
        "unstake" | "withdraw" | "unstake_all" | "withdraw_all" => "Review unstake",
        _ => "Review transaction",
    };

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    let sign_title = match method_name {
        "deposit_and_stake" | "stake" => "Sign transaction to deposit and stake NEAR?",
        "unstake" | "withdraw" | "unstake_all" | "withdraw_all" => {
            "Sign transaction to unstake NEAR?"
        }
        _ => "Sign transaction",
    };

    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    let sign_title = match method_name {
        "deposit_and_stake" | "stake" => "Sign stake?",
        "unstake" | "withdraw" | "unstake_all" | "withdraw_all" => "Sign unstake?",
        _ => "Sign transaction",
    };

    // Determine the amount source by method semantics.
    let display_amount = match amount_source {
        StakeAmountSource::Deposit => Some(deposit),
        StakeAmountSource::Args => amount_opt,
        StakeAmountSource::NotApplicable => None,
    };
    let mut field_context = stake_fn_call::FieldsContext::new();
    let mut writer: FieldsWriter<'_, { stake_fn_call::MAX_FIELDS }> = FieldsWriter::new();

    stake_fn_call::format(
        &mut prefix.signer_id,
        &mut prefix.receiver_id,
        display_amount,
        gas,
        kind,
        &mut field_context,
        &mut writer,
    );

    ui_display_last_action(review_title, sign_title, &mut writer)
}

/// Render a "last action" review (always uses Sign/Hold-to-sign, never "Next Action").
/// `review_title` is the header shown before the fields; `sign_title` is the confirm label.
fn ui_display_last_action<const N: usize>(
    review_title: &str,
    sign_title: &str,
    writer: &mut FieldsWriter<'_, N>,
) -> bool {
    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    {
        let binding = [review_title];
        let my_review = MultiFieldReview::new(
            writer.get_fields(),
            &binding,
            Some(&EYE),
            sign_title,
            Some(&VALIDATE_14),
            "Reject",
            Some(&CROSSMARK),
        );
        my_review.show()
    }

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    {
        let centered_info = CenteredInfo::new(
            review_title,
            "",
            "",
            Some(&NEAR_LOGO),
            false,
            CenteredInfoStyle::LargeCaseBoldInfo,
            0,
        );
        let tag_values_list = TagValueList::new(writer.get_fields(), 2, false, false);
        let info_longpress = InfoLongPress::new(
            sign_title,
            Some(&NEAR_LOGO),
            "Hold to sign",
            TuneIndex::Error,
        );

        let review: NbglGenericReview = NbglGenericReview::new()
            .add_content(NbglPageContent::CenteredInfo(centered_info))
            .add_content(NbglPageContent::TagValueList(tag_values_list))
            .add_content(NbglPageContent::InfoLongPress(info_longpress));

        let mut show_tx = true;
        let mut status_text = "Transaction rejected";
        let mut confirm = false;
        while show_tx {
            confirm = review.show("Reject");
            show_tx = if confirm {
                status_text = "Transaction signed";
                false
            } else {
                NbglChoice::new()
                    .show(
                        "Reject transaction?",
                        "",
                        "Yes, reject",
                        "Go back to transaction",
                    )
                    .not()
            };
        }

        NbglStatus::new()
            .text(status_text)
            .show(status_text == "Transaction signed");

        confirm
    }
}

pub fn ui_display_common<const N: usize>(
    writer: &mut FieldsWriter<'_, N>,
    params: ActionParams,
) -> bool {
    let mut ordinal_fmt_buf = OrdinalStringBuffer::new();
    let is_last = ordinal_string(&mut ordinal_fmt_buf, params);

    let msg_before = ordinal_fmt_buf.as_str();

    let next_msg = if params.is_nested_delegate {
        "Next Subaction"
    } else {
        "Next Action"
    };

    let last_msg = if params.is_nested_delegate {
        "To NEP366 suffix"
    } else {
        "Sign"
    };

    let msg_after = if is_last { last_msg } else { next_msg };

    #[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
    {
        let binding = [msg_before];

        let my_review = MultiFieldReview::new(
            writer.get_fields(),
            &binding,
            Some(&EYE),
            msg_after,
            Some(&VALIDATE_14),
            "Reject",
            Some(&CROSSMARK),
        );

        my_review.show()
    }

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    {
        let centered_info = CenteredInfo::new(
            msg_before,
            "",
            "",
            Some(&NEAR_LOGO),
            false,
            CenteredInfoStyle::LargeCaseBoldInfo,
            0,
        );
        let tag_values_list = TagValueList::new(writer.get_fields(), 2, false, false);

        let info_button = InfoButton::new(
            msg_after,
            Some(&NEAR_LOGO),
            "Confirm action",
            TuneIndex::Success,
        );

        let info_longpress = InfoLongPress::new(
            msg_after,
            Some(&NEAR_LOGO),
            "Hold to sign",
            TuneIndex::Error,
        );

        let mut review: NbglGenericReview = NbglGenericReview::new()
            .add_content(NbglPageContent::CenteredInfo(centered_info))
            .add_content(NbglPageContent::TagValueList(tag_values_list));

        let last_screen: &str;

        if is_last && !params.is_nested_delegate {
            review = review.add_content(NbglPageContent::InfoLongPress(info_longpress));
            last_screen = "Transaction signed";
        } else {
            review = review.add_content(NbglPageContent::InfoButton(info_button));
            last_screen = "Action confirmed";
        }

        let res = review.show("Reject");
        let status: NbglStatus = NbglStatus::new();
        match res {
            true => {
                status.text(last_screen).show(true);
            }
            false => {
                status.text("Transaction rejected").show(false);
            }
        }
        res
    }
}

/// a buffer, large enough to fit description string and
/// 2 u32 numbers as strings
type OrdinalStringBuffer = Buffer<40>;

fn ordinal_string(fmt_buf: &mut OrdinalStringBuffer, params: ActionParams) -> bool {
    let mut num_out = U32Buffer::default();
    let header = if params.is_nested_delegate {
        "View subaction "
    } else {
        "View action "
    };
    fmt_buf.write_str(header);
    // numtoa_buf has to be at least 10 bytes for u32 (4 bytes) : ok
    fmt_buf.write_str(params.ordinal_action.numtoa_str(10, &mut num_out));
    fmt_buf.write_str(" / ");
    // numtoa_buf has to be at least 10 bytes for u32 (4 bytes) : ok
    fmt_buf.write_str(params.total_actions.numtoa_str(10, &mut num_out));

    params.ordinal_action == params.total_actions
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::app_ui::aliases::FnCallCappedString;

    fn make_args(s: &str) -> FnCallCappedString {
        let mut args = FnCallCappedString::new();
        assert!(args
            .deserialize_with_bytes_count(&mut s.as_bytes(), s.len() as u32)
            .is_ok());
        args
    }

    #[test]
    fn parses_amount_with_json_spacing() {
        let args = r#"{"amount" : "1.25"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(
            result,
            Some(near_token::NearToken::from_yoctonear(
                1_250_000_000_000_000_000_000_000
            ))
        );
    }

    #[test]
    fn parses_amount_with_extra_fields() {
        let args = r#"{"account_id":"alice.near","amount":"1000000000000000000000000"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(result, Some(near_token::NearToken::from_near(1)));
    }

    #[test]
    fn parses_amount_ignores_key_prefix_false_match() {
        let args = r#"{"total_amount":"999000000000000000000000000","amount":"5000000000000000000000000"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(
            result,
            Some(near_token::NearToken::from_yoctonear(
                5_000_000_000_000_000_000_000_000
            ))
        );
    }

    #[test]
    fn parses_decimal_with_24_fractional_digits() {
        let args = r#"{"amount":"1.000000000000000000000001"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(
            result,
            Some(near_token::NearToken::from_yoctonear(
                1_000_000_000_000_000_000_000_001
            ))
        );
    }

    #[test]
    fn rejects_decimal_with_more_than_24_fractional_digits() {
        let args = r#"{"amount":"1.0000000000000000000000001"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(result, None);
    }

    #[test]
    fn parses_zero_amount() {
        let args = r#"{"amount":"0"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(result, Some(near_token::NearToken::from_yoctonear(0)));
    }

    #[test]
    fn rejects_overflow_decimal_amount() {
        let args = r#"{"amount":"340282366920938463463374607431.768211456"}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(result, None);
    }

    #[test]
    fn rejects_trailing_dot_decimal_amount() {
        let args = r#"{"amount":"1."}"#;
        let mut s = make_args(args);
        let result = try_parse_amount_from_args(&mut s);
        assert_eq!(result, None);
    }
}
