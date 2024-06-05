import asyncio

async def tcp_echo_client(message):
    while True :
        reader, writer = await asyncio.open_connection(
            '192.168.2.65', 8123)

        print(f'Send: {message!r}')
        writer.write(message.encode())
        await writer.drain()

        bytes_data_length = await reader.read(2) #read 1 packet of tcp, first packet is  the data size of the next packet
        length = int.from_bytes(bytes_data_length, 'big')
        data = await reader.read(length) #read 1 packet of tcp, this contain data
        print(f'Received: {data.decode()!r}')

        print('Close the connection')
        writer.close()
        await writer.wait_closed()
        # await asyncio.sleep(0.1)

async def main():
    loop = asyncio.get_event_loop()
    task1 = loop.create_task(tcp_echo_client('This is tcp ip packet with long sentence bytes data, hope this message don\'t crash the ESP32'))
    await asyncio.gather(task1)

if __name__ == "__main__" :
    asyncio.run(main())
    