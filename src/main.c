/*******************************************************************************
 *   Ethereum 2 Deposit Application
 *   (c) 2020 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"

#include "glyphs.h"

#include "lido_plugin.h"
#include "glyphs.h"

void dispatch_plugin_calls(int message, void *parameters) {
    PRINTF("Handling message %d\n", message);
    switch (message) {
        case ETH_PLUGIN_INIT_CONTRACT:
            handle_init_contract(parameters);
            break;
        case ETH_PLUGIN_PROVIDE_PARAMETER:
            handle_provide_parameter(parameters);
            break;
        case ETH_PLUGIN_FINALIZE:
            handle_finalize(parameters);
            break;
        case ETH_PLUGIN_PROVIDE_INFO:
            handle_provide_token(parameters);
            break;
        case ETH_PLUGIN_QUERY_CONTRACT_ID:
            handle_query_contract_id(parameters);
            break;
        case ETH_PLUGIN_QUERY_CONTRACT_UI:
            handle_query_contract_ui(parameters);
            break;
        default:
            PRINTF("Unhandled message %d\n", message);
            break;
    }
}

void handle_query_ui_exception(unsigned int *args) {
    switch (args[0]) {
        case ETH_PLUGIN_QUERY_CONTRACT_UI:
            ((ethQueryContractUI_t *) args[1])->result = ETH_PLUGIN_RESULT_ERROR;
            break;
        default:
            break;
    }
}

void call_app_ethereum() {
    unsigned int libcall_params[5];

    libcall_params[0] = (unsigned int) "Ethereum";
    libcall_params[1] = 0x100;
    libcall_params[2] = RUN_APPLICATION;
    libcall_params[3] = (unsigned int) NULL;
#ifdef HAVE_NBGL
    caller_app_t capp;
    const char name[] = APPNAME;
    nbgl_icon_details_t icon_details;
    uint8_t bitmap[sizeof(ICONBITMAP)];

    memcpy(&icon_details, &ICONGLYPH, sizeof(ICONGLYPH));
    memcpy(&bitmap, &ICONBITMAP, sizeof(bitmap));
    icon_details.bitmap = (const uint8_t *) bitmap;
    capp.name = (const char *) name;
    capp.icon = &icon_details;
    libcall_params[4] = (unsigned int) &capp;
#else
    libcall_params[4] = (unsigned int) NULL;
#endif
    os_lib_call((unsigned int *) &libcall_params);
}

__attribute__((section(".boot"))) int main(int arg0) {
    // exit critical section
    __asm volatile("cpsie i");

    // ensure exception will work as planned
    os_boot();

    BEGIN_TRY {
        TRY {
            check_api_level(CX_COMPAT_APILEVEL);

            if (!arg0) {
                // called from dashboard, launch Ethereum app
                call_app_ethereum();
                return 0;
            } else {
                // regular call from ethereum
                unsigned int *args = (unsigned int *) arg0;
                if (args[0] != ETH_PLUGIN_CHECK_PRESENCE) {
                    dispatch_plugin_calls(args[0], (void *) args[1]);
                }
                os_lib_end();
            }
        }
        CATCH_OTHER(e) {
            switch (e) {
                // These exceptions are only generated on handle_query_contract_ui()
                // fall through
                case 0x6502:
                case EXCEPTION_OVERFLOW:
                    handle_query_ui_exception((unsigned int *) arg0);
                    break;
                default:
                    break;
            }
            PRINTF("Exception 0x%x caught\n", e);
        }
        FINALLY {
            // Call `os_lib_end`, go back to the ethereum app.
            os_lib_end();
        }
    }
    END_TRY;

    return 0;
}