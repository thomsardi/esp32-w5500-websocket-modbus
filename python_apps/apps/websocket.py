#!/usr/bin/env python
import asyncio
from websockets.server import serve

async def echo(websocket):
    async for message in websocket:
        print(message)
        await websocket.send(message)

async def set_server(cb, ip, port) :
    async with serve(cb, ip, port):
        await asyncio.Future()  # run forever

async def main():
    ip_server = "192.168.2.113"
    port_server = 8000
    loop = asyncio.get_event_loop()
    task1 = loop.create_task(set_server(echo, ip_server, port_server))
    await asyncio.gather(task1)

if __name__ == "__main__" :
    asyncio.run(main())