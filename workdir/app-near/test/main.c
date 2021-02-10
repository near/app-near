// MinUnit (http://www.jera.com/techinfo/jtns/jtn002.html#Source_Code)
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
                               if (message) return message; } while (0)
extern int tests_run;

#include <stdio.h>
#include <string.h>

#include "parse_transaction.h"
#include "constants.h"
#include "context.h"

// Temporary area to sore stuff and reuse the same memory
tmpContext_t tmp_ctx;
uiContext_t ui_context;

// Transactions to test
static const char TRANSFER_1_TRANSACTION_HEX[] = "18000000746573742d636f6e6e6563742d6c65646765722e746573740072068e029a9809c7f2da79c8a274743ff5f7edce24d648bd13e97f7093a03132040000000000000002000000766750dc6e47ddccb2421f27dec206d5ea4694422a1c2fc35ca8c2e3638a02aacf4b0100000003000040bd8b5b936b6c00000000000000";
static const char TRANSFER_2_TRANSACTION_HEX[] = "17000000746573742d70722d3531372d6c65646765722e746573740072068e029a9809c7f2da79c8a274743ff5f7edce24d648bd13e97f7093a0313201000000000000000200000076674241eecd753a851429e4b415eb139dda037f705960200aa7a946bf24469b646e0100000003000000a1edccce1bc2d3000000000000";
static const char TRANSFER_3_TRANSACTION_HEX[] = "40000000746573745f6163635f325f6161616161616161616161616161616161616161616161616161616161616161616161616161616161616161622e746573746e657400c002dcb1dfe53fc741447b28257990e4d907bf5e8a250211e4d4018748b7b247010000000000000040000000746573745f6163635f61616161616161616161616161616161616161616161616161616161616161616161616161616161616161616161622e746573746e6574112dbb2bb983c64df7e1da1b73d3423177f3af489cd22fcd90ce88511b5fc39301000000030000e0f6117ee6d70b1a000000000000";
static const char FUNCTION_CALL_TRANSACTION_HEX[] = "02000000766700dc9f5db36de8b6b403cf579db5425229175dacf5d1c7f10f1a685cbd7cca827e54000000000000000d00000072656365697665722e686572653a79b73517027a94f37203b95b6bb02fd4fb3764f1d689e04adcf54bf372573801000000020b0000006d6574686f645f6e616d650f0000007b2261726773223a2268657265227d00008a5d784563010000004a48011416954508000000000000c185e79a1fa205b9845d992cfa848c0a7bb036a1e0b8a3a4c19fcfb73d03ab5ab1559e8b6b3c37fa6c711159a191ab56be6565512bafc0178ac8326c73c4aa08";
static const char CREATE_ACC_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef60100000000";
static const char DEPLOY_CONTRACT_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef60100000001080000000000000000000000";
static const char STAKE_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef601000000046400000000000000000000000000000000fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec039862979";
static const char ADD_LIMITED_KEY_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef6010000000500fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297900000000000000000001809698000000000000000000000000001700000072616e646f6d5f72656369657665725f69642e6e65617202000000070000006d6574686f6431070000006d6574686f6432";
static const char ADD_UNLIMITED_KEY_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef6010000000500fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec039862979000000000000000000001700000072616e646f6d5f72656369657665725f69642e6e65617202000000070000006d6574686f6431070000006d6574686f6432";
static const char DELETE_KEY_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef6010000000600fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec039862979";
static const char DELETE_ACC_TRANSACTION_HEX[] = "1000000072616e646f6d5f616363312e6e65617200fec7bcd20067927cfab115c72f77c9975b4157e665a0ba3aba382ec03986297901000000000000001000000072616e646f6d5f616363322e6e6561720fa473fd26901df296be6adc4cc4df34d040efa2435224b6986910e630c2fef60100000007110000006f746865724163636f756e742e6e656172";
static const char MULTIPLE_ACCTIONS_TRANSACTION_HEX[] = "02000000766700dc9f5db36de8b6b403cf579db5425229175dacf5d1c7f10f1a685cbd7cca827e55000000000000000d00000072656365697665722e68657265e2f0bc3a1a2e946a6e7b33e645d3aeb7146f1331d1f9e4c8fc7c1970f063925602000000020b0000006d6574686f645f6e616d650f0000007b2261726773223a2268657265227d00008a5d784563010000004a480114169545080000000000030100000000000000000000000000000000f9a87dae39c9f8bcf0f34ebca57b047e2921947ab605ac6b9468083b1ef81f6884c5c0df20c5c7e8faf1204743a8aa7bf348711afb5c04f171991f44fd6ddf03";

int tests_run = 0;

static int parse_hex(char *output_buffer, size_t output_length, const char *hex_str) {
    for (int i = 0; i < output_length; i++) {
        if (hex_str[i * 2] == 0) {
            return i;
        }
        if (hex_str[i * 2 + 1] == 0) {
            return -1;
        }
        sscanf(&hex_str[i * 2], "%2hhx", &output_buffer[i]);
    }
    return -1;
}

#define PARSE_HEX(output_array, hex_str) parse_hex(output_array, sizeof(output_array) / sizeof(output_array[0]), hex_str)

static void print_ui() {
    printf("---\n");
    printf("%s\n", ui_context.line1);
    printf("%s\n", ui_context.line2);
    printf("%s\n", ui_context.line3);
    printf("%s\n", ui_context.long_line);
    printf("%s\n", ui_context.line5);
    printf("%s\n", ui_context.amount);
}

static char *test_parse_transfer_1()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer,TRANSFER_1_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "transfer") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "vg") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "test-connect-ledger.test") == 0);
    mu_assert("parses amount", strcmp(ui_context.amount, "0.002") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_TRANSFER);
    return 0;
}

static char *test_parse_transfer_2()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, TRANSFER_2_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "transfer") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "vg") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "test-pr-517-ledger.test") == 0);
    mu_assert("parses amount", strcmp(ui_context.amount, "1") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_TRANSFER);
    return 0;
}

static char *test_parse_transfer_3()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, TRANSFER_3_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "transfer") == 0);
    // accounts with max Account ID length (64 characters)
    mu_assert("parses receiver", strcmp(ui_context.line2, "test_acc_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab.testnet") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "test_acc_2_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab.testnet") == 0);
    mu_assert("parses amount", strcmp(ui_context.amount, "0.123") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_TRANSFER);
    return 0;
}

static char *test_parse_function_call()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, FUNCTION_CALL_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "method_name") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "receiver.here") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "vg") == 0);
    mu_assert("empty amount", strcmp(ui_context.amount, "") == 0);
    mu_assert("shows JSON args", strcmp(ui_context.long_line, "{\"args\":\"here\"}") == 0);
    mu_assert("parses deposit", strcmp(ui_context.line5, "10") == 0);
    mu_assert("uses function call flow", active_flow == SIGN_FLOW_FUNCTION_CALL);
    return 0;
}

static char *test_parse_create_account()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, CREATE_ACC_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "create account") == 0);
    mu_assert("parses new acc id", strcmp(ui_context.line2, "random_acc2.near") == 0);
    mu_assert("parses master account", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("amount empty", strcmp(ui_context.amount, "") == 0);
    mu_assert("long line empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_GENERIC);
}

static char *test_parse_deploy_contract()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, DEPLOY_CONTRACT_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "deploy contract") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "random_acc2.near") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("amount empty", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_GENERIC);
}

static char *test_parse_stake()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, STAKE_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "stake") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "random_acc2.near") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("amount empty", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_GENERIC);
}

static char *test_parse_add_limited_key()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, ADD_LIMITED_KEY_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "add key") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "random_reciever_id.near") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("amount empty", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("parces limitation", strcmp(ui_context.line5, "0.00000000000000001") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_ADD_FUNCTION_CALL_KEY);
}

static char *test_parse_add_unlimited_key()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, ADD_UNLIMITED_KEY_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "add key") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "random_reciever_id.near") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("amount empty", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("parces Unlimited", strcmp(ui_context.line5, "Unlimited") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_ADD_FUNCTION_CALL_KEY);
}

static char *test_parse_delete_key()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, DELETE_KEY_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "delete key") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "random_acc2.near") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("empty amount", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_GENERIC);
}

static char *test_parse_delete_account()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, DELETE_ACC_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "delete account") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "random_acc2.near") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "random_acc1.near") == 0);
    mu_assert("empty amount", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses transfer flow", active_flow == SIGN_FLOW_GENERIC);
}

static char *test_parse_multiple_actions()
{
    tmp_ctx.signing_context.buffer_used = PARSE_HEX(tmp_ctx.signing_context.buffer, MULTIPLE_ACCTIONS_TRANSACTION_HEX);
    int active_flow = parse_transaction();
    mu_assert("parses action", strcmp(ui_context.line1, "multiple actions") == 0);
    mu_assert("parses receiver", strcmp(ui_context.line2, "receiver.here") == 0);
    mu_assert("parses signer", strcmp(ui_context.line3, "vg") == 0);
    // TODO: Show total amount?
    mu_assert("parses amount", strcmp(ui_context.amount, "") == 0);
    mu_assert("line 4 empty", strcmp(ui_context.long_line, "") == 0);
    mu_assert("line 5 empty", strcmp(ui_context.line5, "") == 0);
    mu_assert("uses generic flow", active_flow == SIGN_FLOW_GENERIC);
    return 0;
}

static char *all_tests()
{
    mu_run_test(test_parse_transfer_1);
    mu_run_test(test_parse_transfer_2);
    mu_run_test(test_parse_transfer_3);
    mu_run_test(test_parse_function_call);
    mu_run_test(test_parse_create_account);
    mu_run_test(test_parse_deploy_contract);
    mu_run_test(test_parse_stake);
    mu_run_test(test_parse_add_limited_key);
    mu_run_test(test_parse_add_unlimited_key);
    mu_run_test(test_parse_delete_key);
    mu_run_test(test_parse_delete_account);
    mu_run_test(test_parse_multiple_actions);
    return 0;
}

int main(int argc, char **argv)
{
    char *result = all_tests();
    if (result != 0) {
        printf("%s\n", result);
    } else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return result != 0;
}