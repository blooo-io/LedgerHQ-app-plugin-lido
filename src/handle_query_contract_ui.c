#include "lido_plugin.h"

// Set UI for the "Send" screen.
static void set_send_ui(ethQueryContractUI_t *msg, lido_parameters_t *context) {
    uint8_t decimals = 0;
    char *ticker;

    switch (context->selectorIndex) {
        case SUBMIT:
            strlcpy(msg->title, "Stake", msg->titleLength);
            decimals = WEI_TO_ETHER;
            ticker = "ETH";
            break;
        case UNWRAP:
            strlcpy(msg->title, "Unwrap", msg->titleLength);
            decimals = WSTETH_DECIMALS;
            ticker = WSTETH_TICKER;
            break;
        case WRAP:
            strlcpy(msg->title, "Wrap", msg->titleLength);
            decimals = STETH_DECIMALS;
            ticker = STETH_TICKER;
            break;
        case REQUEST_WITHDRAWALS_WITH_PERMIT:
            strlcpy(msg->title, "Value", msg->titleLength);
            break;
        default:
            PRINTF("Unhandled selector Index: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }
    switch (context->selectorIndex) {
        case SUBMIT:
            return amountToString(context->amount_sent,
                   msg->pluginSharedRO->txContent->value.length,
                   decimals,
                   ticker,
                   msg->msg,
                   msg->msgLength);
        case UNWRAP:
        case WRAP: 
            return amountToString(context->amount_sent,
                   INT256_LENGTH,
                   decimals,
                   ticker,
                   msg->msg,
                   msg->msgLength);
        case REQUEST_WITHDRAWALS_WITH_PERMIT:
            amountToString(context->amount_sent,
                           INT256_LENGTH,
                           context->decimals_sent,
                           context->ticker_sent,
                           msg->msg,
                           msg->msgLength);
            break;
    }
}

// Set UI for the "Address" screen.
static void set_address_ui(ethQueryContractUI_t *msg, lido_parameters_t *context) {
    switch (context->selectorIndex) {
        case REQUEST_WITHDRAWALS_WITH_PERMIT:
            strlcpy(msg->title, "Owner", msg->titleLength);
            break;
        default:
            PRINTF("Unhandled selector Index: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }

    msg->msg[0] = '0';
    msg->msg[1] = 'x';
    getEthAddressStringFromBinary((uint8_t *) context->address,
                                  msg->msg + 2,
                                  msg->pluginSharedRW->sha3,
                                  0);
}

// Helper function that returns the enum corresponding to the screen that should be displayed.
static screens_t get_screen(ethQueryContractUI_t *msg,
                            lido_parameters_t *context __attribute__((unused))) {
    uint8_t index = msg->screenIndex;

    bool token_received_found = context->tokens_found & TOKEN_RECEIVED_FOUND;

    switch (context->selectorIndex) {
        case SUBMIT:
        case WRAP:
        case UNWRAP:
            switch (index) {
                case 0:
                    return SEND_SCREEN;
                default:
                    return ERROR;
            }
        case REQUEST_WITHDRAWALS_WITH_PERMIT:
            switch (index) {
                case 0:
                    return ADDRESS_SCREEN;
                case 1:
                    return SEND_SCREEN;
                default:
                    return ERROR;     
            }
            break;
        default:
            return ERROR;
    }
    return ERROR;
}

void handle_query_contract_ui(void *parameters) {
    ethQueryContractUI_t *msg = (ethQueryContractUI_t *) parameters;
    lido_parameters_t *context = (lido_parameters_t *) msg->pluginContext;

    memset(msg->title, 0, msg->titleLength);
    memset(msg->msg, 0, msg->msgLength);
    msg->result = ETH_PLUGIN_RESULT_OK;

    screens_t screen = get_screen(msg, context);
    switch (screen) {
        case SEND_SCREEN:
            set_send_ui(msg, context);
            break;
        case ADDRESS_SCREEN:
            set_address_ui(msg, context);
            break;
        default:
            PRINTF("Received an invalid screenIndex %d\n", screen);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }
}
