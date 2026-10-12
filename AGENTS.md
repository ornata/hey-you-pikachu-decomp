# Agent entry point

Read [docs/AGENT_WORKFLOW.md](docs/AGENT_WORKFLOW.md) before changing decompiled
source. The source, the Makefile and the build's fallback report are the evidence;
 Keep one source-unit mutation active per checkout.
An emulation pass is not acceptance: require the clean-ROM gate (`make` prints
`OK: ROM matches`).

Use the Ghidra MCP. If its tools are missing or the server is down, do not give
up and do not restart the shared project by hand: run
`python3 tools/ghidra_mcp.py status`, then `start` if it says down, and query it
from the shell with `tools/ghidra_mcp.py call ...` (see
[TOOLS.md](TOOLS.md#ghidra-mcp-behavior-and-calldata-context)).

Compiler-decision tooling (allocation, scheduler, cross-block moves) lives in the
separate ido-diagnostics repository; it is not part of this tree.
