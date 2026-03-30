use crate::{
    parsing::{self},
    sign_ui::common::tx_public_key_context,
    utils::types::elipsis_fields::{ElipsisFields, EllipsisBuffer},
};
#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use ledger_device_sdk::nbgl::Field;
#[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
use ledger_device_sdk::ui::gadgets::Field;
use near_token::TokenBuffer;

use crate::app_ui::{aliases::CappedAccountId, fields_writer::FieldsWriter};

pub struct FieldsContext {
    pub stake_buffer: TokenBuffer,
    pub pub_key_context: tx_public_key_context::FieldsContext,
}

impl FieldsContext {
    pub fn new() -> Self {
        Self {
            stake_buffer: TokenBuffer::new(),
            pub_key_context: tx_public_key_context::FieldsContext::new(),
        }
    }
}
/// action type (1) + Stake (1) + Public Key (1)
const MAX_FIELDS: usize = 3;

pub fn format<'b, 'a: 'b>(
    stake: &parsing::types::Stake,
    field_context: &'a mut FieldsContext,
    writer: &'_ mut FieldsWriter<'b, MAX_FIELDS>,
) {
    field_context
        .pub_key_context
        .format_public_key(&stake.public_key);
    writer.push_fields(ElipsisFields::one(Field {
        name: "Action type",
        value: "Stake",
    }));

    stake
        .stake
        .display_as_buffer(&mut field_context.stake_buffer);
    writer.push_fields(ElipsisFields::one(Field {
        name: "Stake",
        value: field_context.stake_buffer.as_str(),
    }));

    writer.push_fields(ElipsisFields::one(Field {
        name: "Public Key",
        value: field_context.pub_key_context.as_str(),
    }));
}

/// Combined format: signer account + stake amount + validator key.
/// Used by the merged prefix+action review for native Stake transactions.
pub struct CombinedFieldsContext {
    pub signer_display_buf: EllipsisBuffer,
    pub stake_buffer: TokenBuffer,
    pub pub_key_context: tx_public_key_context::FieldsContext,
    pub pool_display_buf: EllipsisBuffer,
}

impl CombinedFieldsContext {
    pub fn new() -> Self {
        Self {
            signer_display_buf: EllipsisBuffer::default(),
            stake_buffer: TokenBuffer::new(),
            pub_key_context: tx_public_key_context::FieldsContext::new(),
            pool_display_buf: EllipsisBuffer::default(),
        }
    }
}

/// Stake account (1-2) + Stake amount (1) + Stake to pool (1)
pub const COMBINED_MAX_FIELDS: usize = 4;

pub fn format_combined<'b, 'a: 'b>(
    signer_id: &'b mut CappedAccountId,
    receiver_id: &'b mut CappedAccountId,
    stake: &parsing::types::Stake,
    field_context: &'a mut CombinedFieldsContext,
    writer: &'_ mut FieldsWriter<'b, COMBINED_MAX_FIELDS>,
) {
    field_context
        .pub_key_context
        .format_public_key(&stake.public_key);

    #[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
    let title = "Stake with account";
    #[cfg(not(any(target_os = "stax", target_os = "flex", target_os = "apex_p")))]
    let title = "Stake with acc";

    let signer_fields =
        ElipsisFields::from_capped_string(signer_id, title, &mut field_context.signer_display_buf);
    writer.push_fields(signer_fields);

    stake
        .stake
        .display_as_buffer(&mut field_context.stake_buffer);
    writer.push_fields(ElipsisFields::one(Field {
        name: "Stake amount",
        value: field_context.stake_buffer.as_str(),
    }));

    let pool_fields = ElipsisFields::from_capped_string(
        receiver_id,
        "Stake to pool",
        &mut field_context.pool_display_buf,
    );
    writer.push_fields(pool_fields);
}
