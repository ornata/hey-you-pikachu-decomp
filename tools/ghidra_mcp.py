#!/usr/bin/env python3
"""Start, check and query the machine-local Ghidra MCP server from a shell.

Agents that were spawned without the Ghidra MCP tools (or whose session started
while the server was down) can still use it through this helper:

    python3 tools/ghidra_mcp.py status
    python3 tools/ghidra_mcp.py start            # only if status says it is down
    python3 tools/ghidra_mcp.py tools
    python3 tools/ghidra_mcp.py call search_symbols_by_name query=func_8003D030
    python3 tools/ghidra_mcp.py call decompile_function --json '{"binary_name": "...", "name": "..."}'
    python3 tools/ghidra_mcp.py stop             # only a server this helper started

The launcher and project are machine-local (see docs/agents/TOOLS.md). Override with:
    GHIDRA_MCP_URL        default http://127.0.0.1:8765/mcp
    GHIDRA_MCP_LAUNCHER   default ~/ghidra-projects/hey-you-pikachu/start-mcp.sh
    GHIDRA_MCP_LOG        default $TMPDIR/ghidra-mcp.log

Prefer read-only tools. The server also exposes mutation tools (prototype, type,
name, comment edits, import/delete); do not call them on the shared project
unless the task requires it. Starting a stopped server with the configured
launcher is fine; do not delete the project, its lock file, or re-import
binaries.
"""
import argparse
import json
import os
import signal
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

URL = os.environ.get('GHIDRA_MCP_URL', 'http://127.0.0.1:8765/mcp')
LAUNCHER = os.path.expanduser(os.environ.get(
    'GHIDRA_MCP_LAUNCHER', '~/ghidra-projects/hey-you-pikachu/start-mcp.sh'))
LOG = os.environ.get('GHIDRA_MCP_LOG', os.path.join(tempfile.gettempdir(), 'ghidra-mcp.log'))
PIDFILE = os.path.join(tempfile.gettempdir(), 'ghidra-mcp.pid')
PROTOCOL = '2025-03-26'
HEADERS = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream'}


class McpError(Exception):
    pass


def _post(body, session=None, timeout=60):
    headers = dict(HEADERS)
    if session:
        headers['Mcp-Session-Id'] = session
    request = urllib.request.Request(URL, data=json.dumps(body).encode(), headers=headers, method='POST')
    return urllib.request.urlopen(request, timeout=timeout)


def _read_reply(response, request_id):
    """Return the JSON-RPC message with request_id from a JSON or SSE reply."""
    content_type = response.headers.get('Content-Type', '')
    if 'text/event-stream' not in content_type:
        raw = response.read()
        return json.loads(raw) if raw.strip() else None
    for raw in response:
        line = raw.decode('utf-8', 'replace').strip()
        if not line.startswith('data:'):
            continue
        try:
            message = json.loads(line[5:].strip())
        except ValueError:
            continue
        if isinstance(message, dict) and message.get('id') == request_id:
            return message
    return None


class Session:
    """One initialized MCP streamable-HTTP session."""

    def __init__(self):
        self.id = None
        self.next_id = 0

    def __enter__(self):
        reply = self.request('initialize', {
            'protocolVersion': PROTOCOL, 'capabilities': {},
            'clientInfo': {'name': 'hey-you-pikachu-ghidra-mcp-helper', 'version': '1'}})
        if 'error' in reply:
            raise McpError('initialize failed: %s' % json.dumps(reply['error']))
        _post({'jsonrpc': '2.0', 'method': 'notifications/initialized'}, self.id).close()
        return self

    def __exit__(self, *exc):
        if self.id:
            try:
                request = urllib.request.Request(URL, headers={'Mcp-Session-Id': self.id}, method='DELETE')
                urllib.request.urlopen(request, timeout=5).close()
            except Exception:
                pass

    def request(self, method, params=None):
        self.next_id += 1
        body = {'jsonrpc': '2.0', 'id': self.next_id, 'method': method}
        if params is not None:
            body['params'] = params
        try:
            response = _post(body, self.id)
        except urllib.error.HTTPError as error:
            raise McpError('HTTP %d from %s: %s' % (error.code, URL, error.read()[:300].decode('utf-8', 'replace')))
        with response:
            if self.id is None:
                self.id = response.headers.get('Mcp-Session-Id')
            reply = _read_reply(response, self.next_id)
        if reply is None:
            raise McpError('no reply to %s' % method)
        return reply


def is_up():
    try:
        with Session():
            return True
    except Exception:
        return False


def cmd_status(_):
    if is_up():
        print('up: %s' % URL)
        return 0
    print('down: nothing answering MCP at %s' % URL)
    print('start it with: python3 tools/ghidra_mcp.py start')
    return 1


def cmd_start(args):
    if is_up():
        print('already up: %s' % URL)
        return 0
    if not os.path.exists(LAUNCHER):
        print('launcher not found: %s\nSet GHIDRA_MCP_LAUNCHER, or ask the maintainer (see docs/agents/TOOLS.md).' % LAUNCHER,
              file=sys.stderr)
        return 2
    log = open(LOG, 'ab')
    process = subprocess.Popen(['/bin/sh', LAUNCHER], stdout=log, stderr=log, stdin=subprocess.DEVNULL,
                               start_new_session=True)
    with open(PIDFILE, 'w') as handle:
        handle.write(str(process.pid))
    print('launched %s (pid %d), log: %s' % (LAUNCHER, process.pid, LOG))
    deadline = time.time() + args.wait
    while time.time() < deadline:
        if process.poll() is not None:
            print('launcher exited with %s; last log lines:' % process.returncode, file=sys.stderr)
            subprocess.call(['tail', '-n', '25', LOG], stdout=sys.stderr)
            return 3
        if is_up():
            print('up: %s' % URL)
            return 0
        time.sleep(2)
    print('not answering after %ds; it may still be analyzing. Check: tail -f %s' % (args.wait, LOG),
          file=sys.stderr)
    return 4


def cmd_stop(_):
    try:
        pid = int(open(PIDFILE).read().strip())
    except (OSError, ValueError):
        print('no server started by this helper (no %s); stop it where it was launched.' % PIDFILE)
        return 1
    try:
        os.killpg(os.getpgid(pid), signal.SIGTERM)
    except ProcessLookupError:
        print('pid %d is not running' % pid)
    else:
        print('sent SIGTERM to process group of pid %d' % pid)
    os.unlink(PIDFILE)
    return 0


def cmd_tools(_):
    with Session() as session:
        reply = session.request('tools/list')
    for tool in reply.get('result', {}).get('tools', []):
        properties = tool.get('inputSchema', {}).get('properties', {})
        required = set(tool.get('inputSchema', {}).get('required', []))
        arguments = ', '.join(('%s*' if name in required else '%s') % name for name in properties)
        description = (tool.get('description') or '').strip().splitlines()
        print('%s(%s)\n    %s' % (tool['name'], arguments, description[0] if description else ''))
    return 0


def parse_arguments(args):
    arguments = json.loads(args.json) if args.json else {}
    for pair in args.pairs:
        key, separator, value = pair.partition('=')
        if not separator:
            raise SystemExit('argument must be key=value: %r' % pair)
        try:
            arguments[key] = json.loads(value)
        except ValueError:
            arguments[key] = value
    return arguments


def cmd_call(args):
    arguments = parse_arguments(args)
    with Session() as session:
        reply = session.request('tools/call', {'name': args.tool, 'arguments': arguments})
    if 'error' in reply:
        print(json.dumps(reply['error'], indent=2), file=sys.stderr)
        return 1
    result = reply.get('result', {})
    for item in result.get('content', []):
        if item.get('type') == 'text':
            print(item['text'])
        else:
            print(json.dumps(item))
    return 1 if result.get('isError') else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    sub.add_parser('status', help='report whether the MCP endpoint answers').set_defaults(run=cmd_status)
    start = sub.add_parser('start', help='launch the configured server if it is down and wait for it')
    start.add_argument('--wait', type=int, default=180, help='seconds to wait for the endpoint (default 180)')
    start.set_defaults(run=cmd_start)
    sub.add_parser('stop', help='stop a server started by this helper').set_defaults(run=cmd_stop)
    sub.add_parser('tools', help='list the server tools and their arguments').set_defaults(run=cmd_tools)
    call = sub.add_parser('call', help='call one tool')
    call.add_argument('tool')
    call.add_argument('pairs', nargs='*', help='key=value (value parsed as JSON, else a string)')
    call.add_argument('--json', help='full arguments object as JSON')
    call.set_defaults(run=cmd_call)
    args = parser.parse_args()
    try:
        return args.run(args)
    except (McpError, urllib.error.URLError, OSError) as error:
        print('error: %s\nIs the server up? python3 tools/ghidra_mcp.py status' % error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
