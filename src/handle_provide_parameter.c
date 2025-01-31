#include "lido_plugin.h"

// Store the amount sent in the form of a string, without any ticker or
// decimals. These will be added right before display.
static void handle_amount_sent(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    copy_parameter(context->amount_sent, msg->parameter, INT256_LENGTH);
}

static void handle_amount_sent_two(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    copy_parameter(context->amount_sent_two, msg->parameter, INT256_LENGTH);
}

static void handle_amount_length(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    if (!U2BE_from_parameter(msg->parameter, &context->amount_length) ||
        context->amount_length == 0) {
        msg->result = ETH_PLUGIN_RESULT_ERROR;
    }
}

static void handle_address_sent(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    copy_address(context->address_sent, msg->parameter, sizeof(context->address_sent));
}

static void handle_submit(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    // ABI for submit is: submit(address referral)
    if (context->next_param == REFERRAL) {
        // Should be done parsing
        context->next_param = NONE;
    } else {
        PRINTF("Param not supported\n");
        msg->result = ETH_PLUGIN_RESULT_ERROR;
    }
}

static void handle_wrap(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    // ABI for wrap is: wrap(uint256 amount)
    // ABI for unwrap is: unwrap(uint256 amount)
    switch (context->next_param) {
        case AMOUNT_SENT:
            handle_amount_sent(msg, context);
            // Should be done parsing
            context->next_param = NONE;
            break;
        default:
            PRINTF("Param not supported\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

static void handle_permit(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    switch (context->next_param) {
        case ADDRESS_SENT:
            handle_address_sent(msg, context);
            context->next_param = AMOUNT_SENT;
            break;
        case AMOUNT_SENT:
            handle_amount_sent(msg, context);
            context->next_param = NONE;
            // rest of the transaction doesnt need to be displayed on ledger ( permit signature )
            break;
        case NONE:
            break;
        default:
            PRINTF("Param not supported\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

static void handle_request_withdrawals(ethPluginProvideParameter_t *msg,
                                       lido_parameters_t *context) {
    switch (context->next_param) {
        case ADDRESS_SENT:
            handle_address_sent(msg, context);
            context->next_param = AMOUNT_LENGTH;
            break;
        case AMOUNT_LENGTH:
            handle_amount_length(msg, context);
            context->next_param = AMOUNT_SENT;
            break;
        case AMOUNT_SENT:
            handle_amount_sent(msg, context);
            if (context->amount_length > 1) {
                context->next_param = AMOUNT_SENT_TWO;
            } else {
                context->next_param = NONE;
            }
            break;
        case AMOUNT_SENT_TWO:
            handle_amount_sent_two(msg, context);
            context->next_param = NONE;
            break;
        case NONE:
            break;
        default:
            PRINTF("Param not supported\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

static void handle_claim_withdrawals(ethPluginProvideParameter_t *msg, lido_parameters_t *context) {
    switch (context->next_param) {
        case AMOUNT_LENGTH:
            handle_amount_length(msg, context);
            context->next_param = AMOUNT_SENT;
            break;
        case AMOUNT_SENT:
            handle_amount_sent(msg, context);
            if (context->amount_length > 1) {
                context->next_param = AMOUNT_SENT_TWO;
            } else {
                context->next_param = NONE;
            }
            break;
        case AMOUNT_SENT_TWO:
            handle_amount_sent_two(msg, context);
            context->next_param = NONE;
            // rest of the transaction doesnt need to be displayed on ledger (hints)
            break;
        case NONE:
            break;
        default:
            PRINTF("Param not supported\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

void handle_provide_parameter(void *parameters) {
    ethPluginProvideParameter_t *msg = (ethPluginProvideParameter_t *) parameters;
    lido_parameters_t *context = (lido_parameters_t *) msg->pluginContext;
    PRINTF("plugin provide parameter %d %.*H\n",
           msg->parameterOffset,
           PARAMETER_LENGTH,
           msg->parameter);

    msg->result = ETH_PLUGIN_RESULT_OK;

    // If not used remove from here
    if (context->skip) {
        // Skip this step, and don't forget to decrease skipping counter.
        context->skip--;
    } else {
        if ((context->offset) && msg->parameterOffset != context->checkpoint + context->offset) {
            PRINTF("offset: %d, checkpoint: %d, parameterOffset: %d\n",
                   context->offset,
                   context->checkpoint,
                   msg->parameterOffset);
            return;
        }
        context->offset = 0;
        switch (context->selectorIndex) {
            case SUBMIT:
                handle_submit(msg, context);
                break;
            case UNWRAP:
            case WRAP:
                handle_wrap(msg, context);
                break;
            case REQUEST_WITHDRAWALS_WITH_PERMIT:
            case REQUEST_WITHDRAWALS_WSTETH_WITH_PERMIT:
                handle_permit(msg, context);
                break;
            case CLAIM_WITHDRAWALS:
                handle_claim_withdrawals(msg, context);
                break;
            case REQUEST_WITHDRAWALS:
            case REQUEST_WITHDRAWALS_WSTETH:
                handle_request_withdrawals(msg, context);
                break;
            default:
                PRINTF("Selector Index %d not supported\n", context->selectorIndex);
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                break;
        }
        // set valid to true after parsing all parameters
        context->valid = 1;
    }
}