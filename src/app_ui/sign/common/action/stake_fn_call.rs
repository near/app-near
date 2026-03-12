use crate::utils::types::elipsis_fields::{ElipsisFields, EllipsisBuffer};
#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use ledger_device_sdk::nbgl::Field;
#[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
use ledger_device_sdk::ui::gadgets::Field;

use near_gas::GasBuffer;
use near_token::TokenBuffer;

use crate::app_ui::{aliases::CappedAccountId, fields_writer::FieldsWriter};

pub struct FieldsContext {
    pub signer_display_buf: EllipsisBuffer,
    pub pool_display_buf: EllipsisBuffer,
    pub amount_buffer: TokenBuffer,
    pub gas_buf: GasBuffer,
}

impl FieldsContext {
    pub fn new() -> Self {
        Self {
            signer_display_buf: EllipsisBuffer::default(),
            pool_display_buf: EllipsisBuffer::default(),
            amount_buffer: TokenBuffer::new(),
            gas_buf: GasBuffer::new(),
        }
    }
}

/// Stake account (1-2) + amount or "all" (1) + Stake pool (1-2) + Gas (1)
pub const MAX_FIELDS: usize = 6;

/// Labels and amount display depend on the operation kind.
#[derive(Copy, Clone)]
pub enum StakeOpKind {
    /// deposit_and_stake: amount is the attached deposit
    DepositAndStake,
    /// unstake / withdraw: amount is from args JSON
    Unstake,
    /// unstake_all / withdraw_all: no specific amount
    UnstakeAll,
}

pub fn format<'b, 'a: 'b>(
    signer_id: &'b mut CappedAccountId,
    receiver_id: &'b mut CappedAccountId,
    amount_opt: Option<near_token::NearToken>,
    gas: near_gas::NearGas,
    kind: StakeOpKind,
    field_context: &'a mut FieldsContext,
    writer: &'_ mut FieldsWriter<'b, MAX_FIELDS>,
) {
    match kind {
        StakeOpKind::DepositAndStake => {
            let signer_fields = ElipsisFields::from_capped_string(
                signer_id,
                "Stake with account",
                &mut field_context.signer_display_buf,
            );
            writer.push_fields(signer_fields);

            if let Some(amount) = amount_opt {
                amount.display_as_buffer(&mut field_context.amount_buffer);
                writer.push_fields(ElipsisFields::one(Field {
                    name: "Stake amount",
                    value: field_context.amount_buffer.as_str(),
                }));
            }

            let pool_fields = ElipsisFields::from_capped_string(
                receiver_id,
                "Stake to pool",
                &mut field_context.pool_display_buf,
            );
            writer.push_fields(pool_fields);
        }
        StakeOpKind::Unstake => {
            let pool_fields = ElipsisFields::from_capped_string(
                receiver_id,
                "Unstake from pool",
                &mut field_context.pool_display_buf,
            );
            writer.push_fields(pool_fields);

            if let Some(amount) = amount_opt {
                amount.display_as_buffer(&mut field_context.amount_buffer);
                writer.push_fields(ElipsisFields::one(Field {
                    name: "Unstake amount",
                    value: field_context.amount_buffer.as_str(),
                }));
            }

            let signer_fields = ElipsisFields::from_capped_string(
                signer_id,
                "Unstake to",
                &mut field_context.signer_display_buf,
            );
            writer.push_fields(signer_fields);
        }
        StakeOpKind::UnstakeAll => {
            let pool_fields = ElipsisFields::from_capped_string(
                receiver_id,
                "Unstake from pool",
                &mut field_context.pool_display_buf,
            );
            writer.push_fields(pool_fields);

            writer.push_fields(ElipsisFields::one(Field {
                name: "Unstake amount",
                value: "All staked NEAR",
            }));

            let signer_fields = ElipsisFields::from_capped_string(
                signer_id,
                "Unstake to",
                &mut field_context.signer_display_buf,
            );
            writer.push_fields(signer_fields);
        }
    }

    gas.display_as_buffer(&mut field_context.gas_buf);
    writer.push_fields(ElipsisFields::one(Field {
        name: "Gas",
        value: field_context.gas_buf.as_str(),
    }));
}
