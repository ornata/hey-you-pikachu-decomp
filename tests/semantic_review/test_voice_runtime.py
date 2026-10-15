"""VM action provenance distinguishes script playback from ambient action calls."""

from tools.semantic_review.emulator.voice_runtime import action_context


def test_action_context_preserves_original_caller_and_vm_handler_state():
    registers = [0] * 32
    registers[5] = 168
    registers[31] = 0xFFFFFFFF80116838
    vm = 0x80280000
    words = {vm + 0x2C0: 2, vm + 0x2C4: 1}
    assert action_context(registers, vm, words.__getitem__) == {
        "action_id": 168,
        "caller_ra": 0x80116838,
        "vm_action_selector": 2,
        "action_initializing": 1,
    }


def test_action_context_without_vm_does_not_invent_script_provenance():
    registers = [0] * 32
    registers[5] = 87
    registers[31] = 0x80012600

    def read_word(address):
        raise AssertionError("invalid VM must not be read")

    assert action_context(registers, 0, read_word) == {
        "action_id": 87,
        "caller_ra": 0x80012600,
    }


def test_cleanup_removes_only_owned_execution_addresses():
    from types import SimpleNamespace
    from tools.semantic_review.emulator.voice_runtime import remove_owned_breakpoints

    removed = []
    core = SimpleNamespace(
        lib=SimpleNamespace(
            DebugBreakpointCommand=lambda command, address, pointer: removed.append(
                (command, address, pointer)
            )
            or 0
        ),
        check=lambda code, operation: None,
    )
    remove_owned_breakpoints(core, {0x80116838, 0x800409CC})
    assert removed == [(4, 0x800409CC, None), (4, 0x80116838, None)]
