from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import Generator, Optional, List, Union
from ragger.navigator import NavInsID, Navigator

from ragger.backend.interface import RAPDU, BackendInterface
from common import ROOT_SCREENSHOT_PATH


CLA = 0x80
INS_GET_APP_CONFIGURATION = 0x06
INS_GET_PUBKEY = 0x04

# Parameter not used for this APDU
P1_P2_NOT_USED = 0x57
# Parameter 1 for screen confirmation for GET_PUBKEY.
P1_CONFIRM = 0x00

# Return codes
SW_OK = 0x9000
SW_DENY = 0x6985

FINISH_STUB_APDU = RAPDU(0xFFFF, bytes())


@dataclass(frozen=True)
class NavigableConditions:
    value: List[str]


@dataclass(frozen=True)
class AsyncAPDU:
    data: bytes
    navigable_conditions: NavigableConditions
    expected_response: RAPDU


@dataclass
class Nearbackend:
    backend: BackendInterface

    def get_version(self) -> RAPDU:
        return self.backend.exchange(
            CLA, INS_GET_APP_CONFIGURATION, P1_P2_NOT_USED, P1_P2_NOT_USED, bytes()
        )

    @contextmanager
    def get_public_key_with_confirmation(self, path: bytes) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            CLA, INS_GET_PUBKEY, P1_CONFIRM, P1_P2_NOT_USED, path
        ) as response:
            yield response

    def get_async_response(self) -> Optional[RAPDU]:
        return self.backend.last_async_response

    def sign_message_chunks(
        self, chunks: List[Union[bytes, AsyncAPDU]]
    ) -> Generator[Union[NavigableConditions, RAPDU], None, RAPDU]:
        for chunk in chunks:
            if isinstance(chunk, AsyncAPDU):
                with self.backend.exchange_async_raw(chunk.data):
                    yield chunk.navigable_conditions
                yield chunk.expected_response
            elif isinstance(chunk, bytes):
                rapdu = self.backend.exchange_raw(chunk)
                if rapdu.status != SW_OK:
                    return rapdu
            else:
                raise TypeError("bytes or AsyncAPDU expected")
        return FINISH_STUB_APDU

def condition_folder_name(event_index: int, additional_index: bool, condition_index: int):
    if additional_index:
        return str(event_index) + "_" + str(condition_index)
    return str(event_index)


def navigate_conditions(
    navigator: Navigator,
    conditions: NavigableConditions,
    event_index: int,
    test_name,
    firmware,
):
    reject_labels = {"Reject", "Cancel"}
    for cond_index, condition in enumerate(conditions.value):
        str_index = condition_folder_name(event_index, len(conditions.value) > 1, cond_index)
        condition_folder = Path(test_name) / (
            str_index + "_" + condition.lower().replace(" ", "_").replace("!", "_bang")
        )
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(
                NavInsID.RIGHT_CLICK,
                [NavInsID.BOTH_CLICK],
                condition,
                ROOT_SCREENSHOT_PATH,
                condition_folder,
                screen_change_after_last_instruction=False,
            )
        else:
            if condition == "Sign":
                navigator.navigate_until_text_and_compare(
                    NavInsID.USE_CASE_REVIEW_TAP,
                    [
                        NavInsID.USE_CASE_REVIEW_CONFIRM
                    ],
                    condition,
                    ROOT_SCREENSHOT_PATH,
                    condition_folder,
                    screen_change_after_last_instruction=True,
                )
            elif condition in reject_labels:
                navigator.navigate_until_text_and_compare(
                    NavInsID.USE_CASE_REVIEW_TAP,
                    [NavInsID.USE_CASE_REVIEW_REJECT, NavInsID.USE_CASE_CHOICE_CONFIRM],
                    condition,
                    ROOT_SCREENSHOT_PATH,
                    condition_folder,
                    screen_change_after_last_instruction=True,
                )
            else:
                navigator.navigate_until_text_and_compare(
                    NavInsID.USE_CASE_REVIEW_TAP,
                    [
                        NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                    ],
                    condition,
                    ROOT_SCREENSHOT_PATH,
                    condition_folder,
                    screen_change_after_last_instruction=True,
                )


def assert_async_response(client: Nearbackend, expected_response: RAPDU):
    response = client.get_async_response()

    assert response.status == expected_response.status  # type: ignore
    assert response.data == expected_response.data  # type: ignore


def generic_test_sign(
    client: Nearbackend,
    chunks: List[Union[bytes, AsyncAPDU]],
    navigator: Navigator,
    test_name,
    firmware,
):
    numbered_chunks = enumerate(client.sign_message_chunks(chunks))

    try:
        while True:
            index, chunk_event = next(numbered_chunks)
            if isinstance(chunk_event, NavigableConditions):
                navigate_conditions(navigator, chunk_event, index, test_name, firmware)
            elif isinstance(chunk_event, RAPDU):
                assert_async_response(client, chunk_event)
    except StopIteration as e:
        if e.value != FINISH_STUB_APDU:
            raise AssertionError(e.value) from e


def generic_test_sign_single_review(
    client: Nearbackend,
    chunks: List[Union[bytes, AsyncAPDU]],
    navigator: Navigator,
    test_name,
    firmware,
):
    numbered_chunks = enumerate(client.sign_message_chunks(chunks))
    saw_review = False

    try:
        while True:
            index, chunk_event = next(numbered_chunks)
            if isinstance(chunk_event, NavigableConditions):
                assert not saw_review
                assert len(chunk_event.value) == 1

                saw_review = True
                navigate_conditions(navigator, chunk_event, index, test_name, firmware)
            elif isinstance(chunk_event, RAPDU):
                assert_async_response(client, chunk_event)
    except StopIteration as e:
        assert saw_review
        if e.value != FINISH_STUB_APDU:
            raise AssertionError(e.value) from e
