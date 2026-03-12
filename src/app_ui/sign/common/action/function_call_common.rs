use crate::{
    parsing,
    utils::types::elipsis_fields::{ElipsisFields, EllipsisBuffer},
};
#[cfg(any(target_os = "stax", target_os = "flex", target_os = "apex_p"))]
use ledger_device_sdk::nbgl::Field;
#[cfg(any(target_os = "nanox", target_os = "nanosplus"))]
use ledger_device_sdk::ui::gadgets::Field;

use near_gas::GasBuffer;
use near_token::TokenBuffer;

use crate::app_ui::fields_writer::FieldsWriter;

pub struct FieldsContext {
    pub method_name_display_buf: EllipsisBuffer,
    pub gas_buf: GasBuffer,
    pub deposit_buf: TokenBuffer,
}

impl FieldsContext {
    pub fn new() -> Self {
        Self {
            method_name_display_buf: EllipsisBuffer::default(),
            gas_buf: GasBuffer::new(),
            deposit_buf: TokenBuffer::new(),
        }
    }
}
pub fn format<'b, 'a: 'b, const N: usize>(
    func_call_common: &'a mut parsing::types::FunctionCallCommon,
    field_context: &'a mut FieldsContext,
    writer: &'_ mut FieldsWriter<'b, N>,
) {
    writer.push_fields(ElipsisFields::one(Field {
        name: "Action type",
        value: "Function Call",
    }));

    let method_name = ElipsisFields::from_capped_string(
        &mut func_call_common.method_name,
        "Method Name",
        &mut field_context.method_name_display_buf,
    );

    writer.push_fields(method_name);

    func_call_common
        .gas
        .display_as_buffer(&mut field_context.gas_buf);
    writer.push_fields(ElipsisFields::one(Field {
        name: "Gas",
        value: field_context.gas_buf.as_str(),
    }));

    func_call_common
        .deposit
        .display_as_buffer(&mut field_context.deposit_buf);
    writer.push_fields(ElipsisFields::one(Field {
        name: "Deposit",
        value: field_context.deposit_buf.as_str(),
    }));
}
