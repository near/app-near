from application_client.client import (
    AsyncAPDU,
    SW_OK,
    NavigableConditions,
    Nearbackend,
    FINISH_STUB_APDU,
    generic_test_sign,
)
from ragger.backend.interface import RAPDU
from ragger.backend import RaisePolicy
from ragger.navigator import Navigator


def test_sign_wrong_pubkey_transfer(firmware, backend, navigator: Navigator, test_name):
    """
    Transaction {
        signer_id: AccountId(
            "1b11b3b31673033936ad07bddc01f9da27d974811e480fb197c799e23480a489",
        ),
        public_key: ed25519:2pffV8fXgXUyuKdTS8Nqnvbkua16wEtTv8tdWZCpVtat,
        nonce: 103595482000005,
        receiver_id: AccountId(
            "dc7e34eecec3096a4a661e10932834f801149c49dba9b93322f6d9de18047f9c",
        ),
        block_hash: Cb3vKNiF3MUuVoqfjuEFCgSNPT79pbuVfXXd2RxDXc5E,
        actions: [
            Transfer(
                TransferAction {
                    deposit: 150000000000000000000000,
                },
            ),
        ],
    }
    """
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    client = Nearbackend(backend)
    chunks = [
        AsyncAPDU(
            data=bytes.fromhex(
                "80028057fa8000002c8000018d8000000080000000800000014000000031623131623362333136373330333339333661643037626464633031663964613237643937343831316534383066623139376337393965323334383061343839001b11b3b31673033936ad07bddc01f9da27d974811e480fb197c799e23480a48985aae733385e00004000000064633765333465656365633330393661346136363165313039333238333466383031313439633439646261396239333332326636643964653138303437663963ac299ac1376e375cd39338d8b29225613ef947424b74a3207c1226863a72583101000000030000c071f0d12b84c31f000000000000"
            ),
            navigable_conditions=NavigableConditions(
                value=["Error!"],
            ),
            expected_response=RAPDU(
                0xB00D,
                bytes(),
            ),
        )
    ]
    generic_test_sign(client, chunks, navigator, test_name, firmware)


def test_sign_wrong_pubkey_delegate_action_transfer(
    firmware, backend, navigator: Navigator, test_name
):
    """
    DelegateAction {
        sender_id: AccountId(
            "bob.near",
        ),
        receiver_id: AccountId(
            "alice.near",
        ),
        actions: [
            NonDelegateAction(
                Transfer(
                    TransferAction {
                        deposit: 150000000000000000000000,
                    },
                ),
            ),
        ],
        nonce: 127127122121,
        max_block_height: 100500,
        public_key: ed25519:2pffV8fXgXUyuKdTS8Nqnvbkua16wEtTv8tdWZCpVtat,
    }
    """
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    client = Nearbackend(backend)
    chunks = [
        AsyncAPDU(
            data=bytes.fromhex(
                "80088057748000002c8000018d80000000800000008000000108000000626f622e6e6561720a000000616c6963652e6e65617201000000030000c071f0d12b84c31f000000000000c9f05d991d0000009488010000000000001b11b3b31673033936ad07bddc01f9da27d974811e480fb197c799e23480a489"
            ),
            navigable_conditions=NavigableConditions(
                value=["Proceed to subactions", "To NEP366 suffix", "Sign", "Error!"],
            ),
            expected_response=RAPDU(
                0xB00D,
                bytes(),
            ),
        )
    ]
    generic_test_sign(client, chunks, navigator, test_name, firmware)
