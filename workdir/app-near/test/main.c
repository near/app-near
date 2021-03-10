// clang-format off
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
// clang-format on

#include <stdio.h>
#include "constants.h"
#include "context.h"
#include "parse_transaction.h"

// Temporary area to store stuff and reuse the same memory
tmpContext_t tmp_ctx;
uiContext_t ui_context;

static size_t load_testcase(const char *filename, uint8_t *buffer) {
  FILE *f = fopen(filename, "rb");
  assert_non_null(f);

  size_t filesize = fread(buffer, 1, MAX_DATA_SIZE, f);
  assert_non_null(feof(f));
  fclose(f);
  return filesize;
}

static void test_parse_transfer_1(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/transfer_1_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "transfer");                  // action
  assert_string_equal(ui_context.line2, "vg");                        // receiver
  assert_string_equal(ui_context.line3, "test-connect-ledger.test");  // signer
  assert_string_equal(ui_context.amount, "0.002");                    // amount
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_TRANSFER);
}

static void test_parse_transfer_2(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/transfer_2_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "transfer");                 // action
  assert_string_equal(ui_context.line2, "vg");                       // receiver
  assert_string_equal(ui_context.line3, "test-pr-517-ledger.test");  // signer
  assert_string_equal(ui_context.amount, "1");                       // amount
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_TRANSFER);
}

static char test_parse_transfer_3(void **state) {
    (void)state;

    tmp_ctx.signing_context.buffer_used =
        load_testcase("../testcases/transfer_3_transaction.raw",
                    tmp_ctx.signing_context.buffer);
    int active_flow = parse_transaction();

    assert_string_equal(ui_context.line1, "transfer");                         // action
    // accounts with max Account ID length (64 characters)
    assert_string_equal(ui_context.line2,
        "test_acc_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab.testnet");  // receiver
    assert_string_equal(ui_context.line3,
        "test_acc_2_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab.testnet");  // signer
    assert_string_equal(ui_context.amount, "0.123");                          // amount
    assert_string_equal(ui_context.long_line, "");
    assert_string_equal(ui_context.line5, "");
    assert_int_equal(active_flow, SIGN_FLOW_TRANSFER);
}

static void test_parse_function_call(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/function_call_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "method_name");    // action
  assert_string_equal(ui_context.line2, "receiver.here");  // receiver
  assert_string_equal(ui_context.line3, "vg");             // signer
  assert_string_equal(ui_context.amount, "");              // amount
  assert_string_equal(ui_context.long_line,
                      "{\"args\":\"here\"}");   // JSON args
  assert_string_equal(ui_context.line5, "10");  // deposit
  assert_int_equal(active_flow, SIGN_FLOW_FUNCTION_CALL);
}

static void test_parse_create_account(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/create_account_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "create account");    // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // new account id
  assert_string_equal(ui_context.line3, "random_acc1.near");  // master account
  assert_string_equal(ui_context.amount, "");                 // amount
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_deploy_contract(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/deploy_contract_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "deploy contract");   // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_stake(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/stake_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "stake");             // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_add_limited_key(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/add_limited_key_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "add key");                  // action
  assert_string_equal(ui_context.line2, "random_reciever_id.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");         // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "0.00000000000000001");      // limitation
  assert_int_equal(active_flow, SIGN_FLOW_ADD_FUNCTION_CALL_KEY);
}

static void test_parse_add_unlimited_key(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/add_unlimited_key_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "add key");                  // action
  assert_string_equal(ui_context.line2, "random_reciever_id.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");         // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "Unlimited");                // unlimited
  assert_int_equal(active_flow, SIGN_FLOW_ADD_FUNCTION_CALL_KEY);
}

static void test_parse_delete_key(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/delete_key_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "delete key");        // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_delete_account(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/delete_account_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "delete account");    // action
  assert_string_equal(ui_context.line2, "random_acc2.near");  // receiver
  assert_string_equal(ui_context.line3, "random_acc1.near");  // signer
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

static void test_parse_multiple_actions(void **state) {
  (void)state;

  tmp_ctx.signing_context.buffer_used =
      load_testcase("../testcases/multiple_actions_transaction.raw",
                    tmp_ctx.signing_context.buffer);
  int active_flow = parse_transaction();

  assert_string_equal(ui_context.line1, "multiple actions");
  assert_string_equal(ui_context.line2, "receiver.here");  // receiver
  assert_string_equal(ui_context.line3, "vg");             // signer
  // TODO: Show total amount?
  assert_string_equal(ui_context.amount, "");
  assert_string_equal(ui_context.long_line, "");
  assert_string_equal(ui_context.line5, "");
  assert_int_equal(active_flow, SIGN_FLOW_GENERIC);
}

int main() {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_parse_transfer_1),
      cmocka_unit_test(test_parse_transfer_2),
      cmocka_unit_test(test_parse_transfer_3),
      cmocka_unit_test(test_parse_function_call),
      cmocka_unit_test(test_parse_create_account),
      cmocka_unit_test(test_parse_deploy_contract),
      cmocka_unit_test(test_parse_stake),
      cmocka_unit_test(test_parse_add_limited_key),
      cmocka_unit_test(test_parse_add_unlimited_key),
      cmocka_unit_test(test_parse_delete_key),
      cmocka_unit_test(test_parse_delete_account),
      cmocka_unit_test(test_parse_multiple_actions),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
