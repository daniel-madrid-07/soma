#!/usr/bin/env python3
# SOMA — cliente directo del socket de BlenderMCP (puerto 9876). Permite ejecutar
# Python dentro del Blender abierto SIN pasar por la capa MCP (no requiere reiniciar
# Claude). Envía {"type":"execute_code","params":{"code":...}} y devuelve lo impreso.
# Uso:  echo "print('hola')" | python tools/blender/mcp_client.py
#       python tools/blender/mcp_client.py archivo.py
import socket, json, sys

def send(code, host='127.0.0.1', port=9876, timeout=180):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect((host, port))
    s.sendall(json.dumps({"type": "execute_code", "params": {"code": code}}).encode('utf-8'))
    buf = b''
    while True:
        chunk = s.recv(65536)
        if not chunk:
            break
        buf += chunk
        try:
            resp = json.loads(buf.decode('utf-8'))
            s.close()
            return resp
        except json.JSONDecodeError:
            continue
    s.close()
    try:
        return json.loads(buf.decode('utf-8'))
    except Exception:
        return {"status": "error", "raw": buf.decode('utf-8', 'replace')}

if __name__ == '__main__':
    code = open(sys.argv[1], encoding='utf-8').read() if len(sys.argv) > 1 else sys.stdin.read()
    r = send(code)
    res = r.get('result') if isinstance(r, dict) else None
    if isinstance(res, dict):                 # {"executed":True,"result":"<stdout>"}
        sys.stdout.write(res.get('result', json.dumps(res, ensure_ascii=False)))
    elif isinstance(res, str):
        sys.stdout.write(res)
    else:
        sys.stdout.write(json.dumps(r, ensure_ascii=False))
