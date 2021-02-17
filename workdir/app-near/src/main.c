/*******************************************************************************
*   Waves platform Wallet App for Nano Ledger S. Updated By Waves community.
*   Copyright (c) 2017-2018 Sergey Tolmachev (Tolsi) <tolsi.ru@gmail.com>
* 
*   Based on Sample code provided and (c) 2016 Ledger and 2017-2018 Jake B. (Burstcoin)
*
*   Based on Ledger app boilerplate (c) 2016 Ledger
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

#include "utils.h"
#include "ui.h"
#include "get_public_key.h"
#include "sign_transaction.h"
#include "menu.h"
#include "main.h"
#include "near.h"
#include "crypto/ledger_crypto.h"
#include "canary.h"

// Temporary area to sore stuff and reuse the same memory
tmpContext_t tmp_ctx;
uiContext_t ui_context;

// SPI Buffer for io_event
unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

uint32_t deserialize_uint32_t(unsigned char *buffer)
{
    uint32_t value = 0;

    value |= buffer[0] << 24;
    value |= buffer[1] << 16;
    value |= buffer[2] << 8;
    value |= buffer[3];
    return value;
}

// 20 bytes total
void read_path_from_bytes(unsigned char *buffer, uint32_t *path) {
    path[0] = deserialize_uint32_t(buffer);
    path[1] = deserialize_uint32_t(buffer + 4);
    path[2] = deserialize_uint32_t(buffer + 8);
    path[3] = deserialize_uint32_t(buffer + 12);
    path[4] = deserialize_uint32_t(buffer + 16);
}

// Handle a signing request -- called both from the main apdu loop as well as from
// the button handler after the user verifies the transaction.
void add_chunk_data() {
    // if this is a first chunk
    if (tmp_ctx.signing_context.buffer_used == 0) {
        // then there is the bip32 path in the first chunk - first 20 bytes of data
        read_path_from_bytes(&G_io_apdu_buffer[5], (uint32_t *) tmp_ctx.signing_context.bip32);
        int path_size = sizeof(tmp_ctx.signing_context.bip32);

        // Update the other data from this segment
        int data_size = G_io_apdu_buffer[4];
        if (data_size < path_size) {
            // TODO: Have specific error for underflow?
            THROW(SW_BUFFER_OVERFLOW);
        }
        data_size -= path_size;
        PRINTF("data_size: %d\n", data_size);

        os_memmove((char *) tmp_ctx.signing_context.buffer, &G_io_apdu_buffer[25], data_size);
        PRINTF("buffer: %.*h\n", data_size, tmp_ctx.signing_context.buffer);
        tmp_ctx.signing_context.buffer_used += data_size;
    } else {
        // else update the data from entire segment.
        int data_size = G_io_apdu_buffer[4];
        PRINTF("data_size: %d\n", data_size);
        if (data_size > MAX_DATA_SIZE || tmp_ctx.signing_context.buffer_used + data_size > MAX_DATA_SIZE) {
            THROW(SW_BUFFER_OVERFLOW);
        }
        os_memmove((char *) &tmp_ctx.signing_context.buffer[tmp_ctx.signing_context.buffer_used], &G_io_apdu_buffer[5], data_size);
        PRINTF("buffer: %.*h\n", data_size, &tmp_ctx.signing_context.buffer[tmp_ctx.signing_context.buffer_used]);
        tmp_ctx.signing_context.buffer_used += data_size;
    }
}

// like https://github.com/lenondupe/ledger-app-stellar/blob/master/src/main.c#L1784
uint32_t set_result_sign() {
    cx_ecfp_public_key_t public_key;
    cx_ecfp_private_key_t private_key;
    get_keypair_by_path((uint32_t *) tmp_ctx.signing_context.bip32, &public_key, &private_key);

    public_key_le_to_be(&public_key);

    BEGIN_TRY {
        TRY {
            uint8_t signature[64];
            near_message_sign(&private_key, public_key.W, (unsigned char *)tmp_ctx.signing_context.buffer, tmp_ctx.signing_context.buffer_used, signature);

            os_memmove((char *)G_io_apdu_buffer, signature, sizeof(signature));
        } FINALLY {
            // reset all private stuff
            os_memset(&private_key, 0, sizeof(cx_ecfp_private_key_t));
            os_memset(&public_key, 0, sizeof(cx_ecfp_public_key_t));
        }
    }
    END_TRY;

    return 64;
}

#define OFFSET_CLA 0
#define OFFSET_INS 1
#define OFFSET_P1 2
#define OFFSET_P2 3
#define OFFSET_LC 4
#define OFFSET_CDATA 5

// Called by both the U2F and the standard communications channel
void handle_apdu(volatile unsigned int *flags, volatile unsigned int *tx, volatile unsigned int rx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(SW_CLA_NOT_SUPPORTED);
            }

            PRINTF("command: %d\n", G_io_apdu_buffer[OFFSET_INS]);
            switch (G_io_apdu_buffer[OFFSET_INS]) {
            case INS_SIGN: {
                if (G_io_apdu_buffer[OFFSET_LC] != rx - 5) {
                    // the length of the APDU should match what's in the 5-byte header.
                    // If not fail.  Don't want to buffer overrun or anything.
                    THROW(SW_CONDITIONS_NOT_SATISFIED);
                }

                handle_sign_transaction(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
            } break;

            case INS_GET_PUBLIC_KEY: {
                if (G_io_apdu_buffer[OFFSET_LC] != rx - 5 || G_io_apdu_buffer[OFFSET_LC] != 20) {
                    // the length of the APDU should match what's in the 5-byte header.
                    // If not fail.  Don't want to buffer overrun or anything.
                    THROW(SW_CONDITIONS_NOT_SATISFIED);
                }

                handle_get_public_key(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2], G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], flags, tx);
            } break;

            case INS_GET_APP_CONFIGURATION:
                // NOTE: This allows using INS_GET_APP_CONFIGURATION as "reset state" command
                init_context();

                G_io_apdu_buffer[0] = LEDGER_MAJOR_VERSION;
                G_io_apdu_buffer[1] = LEDGER_MINOR_VERSION;
                G_io_apdu_buffer[2] = LEDGER_PATCH_VERSION;
                *tx = 3;
                THROW(SW_OK);
                break;

            default:
                THROW(0x6D00);
                break;
            }
        }
        CATCH(EXCEPTION_IO_RESET) {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e) {
        switch (e & 0xF000) {
            case 0x6000:
                sw = e;
                break;
            case 0x9000:
                // All is well
                sw = e;
                break;
            default:
                // Internal error
                sw = 0x6800 | (e & 0x7FF);
                break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    END_TRY;
    }
}

void init_context() {
    os_memset(&tmp_ctx, 0, sizeof(tmp_ctx));
}

void app_main(void) {
    volatile unsigned int rx = 0;
    volatile unsigned int tx = 0;
    volatile unsigned int flags = 0;

    // DESIGN NOTE: the bootloader ignores the way APDU are fetched. The only
    // goal is to retrieve APDU.
    // When APDU are to be fetched from multiple IOs, like NFC+USB+BLE, make
    // sure the io_event is called with a
    // switch event, before the apdu is replied to the bootloader. This avoid
    // APDU injection faults.
    for (;;) {
        volatile unsigned short sw = 0;
        BEGIN_TRY {
            TRY {
                rx = tx;
                tx = 0; // ensure no race in catch_other if io_exchange throws
                        // an error
                rx = io_exchange(CHANNEL_APDU | flags, rx);
                flags = 0;

                // no apdu received, well, reset the session, and reset the
                // bootloader configuration
                if (rx == 0) {
                    THROW(SW_SECURITY_STATUS_NOT_SATISFIED);
                }

                PRINTF("New APDU received:\n%.*H\n", rx, G_io_apdu_buffer);
                INIT_CANARY;
                handle_apdu(&flags, &tx, rx);
            }
            CATCH(EXCEPTION_IO_RESET) {
              THROW(EXCEPTION_IO_RESET);
            }
            CATCH_OTHER(e) {
                switch (e & 0xF000) {
                    case 0x6000:
                        sw = e;
                        break;
                    case 0x9000:
                        // All is well
                        sw = e;
                        break;
                    default:
                        // Internal error
                        sw = 0x6800 | (e & 0x7FF);
                        break;
                }
                if (e != 0x9000) {
                    flags &= ~IO_ASYNCH_REPLY;
                }
                // Unexpected exception => report
                G_io_apdu_buffer[tx] = sw >> 8;
                G_io_apdu_buffer[tx + 1] = sw;
                tx += 2;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

//return_to_dashboard:
    return;
}

// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t*)element);
}

unsigned char io_event(unsigned char channel) {
    // nothing done with the event, throw an error on the transport layer if
    // needed

    // can't have more than one tag in the reply, not supported yet.
    switch (G_io_seproxyhal_spi_buffer[0]) {
        case SEPROXYHAL_TAG_FINGER_EVENT:
            UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
            UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
            break;

        case SEPROXYHAL_TAG_STATUS_EVENT:
            if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID && !(U4BE(G_io_seproxyhal_spi_buffer, 3) & SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
                THROW(EXCEPTION_IO_RESET);
            }
            // no break is intentional
        default:
            UX_DEFAULT_EVENT();
            break;

        case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
            UX_DISPLAYED_EVENT({});
            break;

        case SEPROXYHAL_TAG_TICKER_EVENT:
            UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer,
            {
#ifndef TARGET_NANOX
                if (UX_ALLOWED) {
                    if (ux_step_count) {
                    // prepare next screen
                    ux_step = (ux_step+1)%ux_step_count;
                    // redisplay screen
                    UX_REDISPLAY();
                    }
                }
#endif // TARGET_NANOX
            });
            break;
    }

    // close the event if not done previously (by a display or whatever)
    if (!io_seproxyhal_spi_is_status_sent()) {
        io_seproxyhal_general_status();
    }

    // command has been processed, DO NOT reset the current APDU transport
    return 1;
}


unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
    switch (channel & ~(IO_FLAGS)) {
        case CHANNEL_KEYBOARD:
            break;

        // multiplexed io exchange over a SPI channel and TLV encapsulated protocol
        case CHANNEL_SPI:
            if (tx_len) {
                io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);

                if (channel & IO_RESET_AFTER_REPLIED) {
                    reset();
                }
                return 0; // nothing received from the master so far (it's a tx
                        // transaction)
            } else {
                return io_seproxyhal_spi_recv(G_io_apdu_buffer,
                                            sizeof(G_io_apdu_buffer), 0);
            }

        default:
            THROW(INVALID_PARAMETER);
    }
    return 0;
}


void app_exit(void) {

    BEGIN_TRY_L(exit) {
        TRY_L(exit) {
            os_sched_exit(-1);
        }
        FINALLY_L(exit) {

        }
    }
    END_TRY_L(exit);
}

void nv_app_state_init(){
    if (N_storage.initialized != 0x01) {
        internalStorage_t storage;
        storage.dummy_setting_1 = 0x00;
        storage.dummy_setting_2 = 0x00;
        storage.initialized = 0x01;
        nvm_write((internalStorage_t*)&N_storage, (void*)&storage, sizeof(internalStorage_t));
    }
    dummy_setting_1 = N_storage.dummy_setting_1;
    dummy_setting_2 = N_storage.dummy_setting_2;
}

__attribute__((section(".boot"))) int main(void) {
    // exit critical section
    __asm volatile("cpsie i");

    init_context();
    // ensure exception will work as planned
    os_boot();

    for (;;) {
        UX_INIT();

        BEGIN_TRY {
            TRY {
                io_seproxyhal_init();

                nv_app_state_init();

                USB_power(0);
                USB_power(1);

                ui_idle();

#ifdef HAVE_BLE
                BLE_power(0, NULL);
                BLE_power(1, "Nano X");
#endif // HAVE_BLE

                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX before continuing
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }
    app_exit();
    return 0;
}
