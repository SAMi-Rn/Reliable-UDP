import socket
import struct
import multiprocessing
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time


colors = [
    'red',
    'green',
    'lightcyan',
    'blue',
    'yellow',
    'orange',
    'purple',
    'pink',
    'gray'
]

packet_types = [
    "SENT_PACKET",
    "RECEIVED_PACKET",
    "RECEIVED_ACK",
    "RESENT_PACKET",
    "DROPPED_CLIENT_PACKET",
    "DELAYED_CLIENT_PACKET",
    "DROPPED_SERVER_PACKET",
    "DELAYED_SERVER_PACKET",
    "CORRUPTED_PACKET"
]

server_names = [
    'Server',
    'Client',
    'Proxy'
]

plt.style.use('dark_background')


def connect_to_server(server_id, host, port, data):
    client_socket = socket.socket()
    try:
        client_socket.connect((host, port))
        while True:
            message = client_socket.recv(4)
            if message:
                value = struct.unpack('<I', message)[0]
                print(f"From {server_names[server_id]}: {packet_types[value]}")
                data[server_id].append((value, time.time()))
    except KeyboardInterrupt:
        pass
    except ConnectionRefusedError:
        print(f"Could not connect to server {server_id} at {host}:{port}")
    finally:
        client_socket.close()


def update_plot(i, ax, server_id, data):
    if data[server_id]:
        ax.clear()

        times = [t for _, t in data[server_id]]
        values = [v for v, _ in data[server_id]]

        packet_count_per_type = {ptype: 0 for ptype in range(len(colors))}

        packet_history = {ptype: [] for ptype in range(len(colors))}

        for value, time in zip(values, times):
            packet_count_per_type[value] += 1
            for ptype in packet_history:
                packet_history[ptype].append(packet_count_per_type[ptype])

        for packet_type in range(len(colors)):
            ax.plot(times, packet_history[packet_type], color=colors[packet_type], label=packet_types[packet_type])

        ax.set_ylabel('Number of Packets')
        ax.set_xlabel('Time')
        ax.set_title(f'Data from {["Server", "Client", "Proxy"][server_id]}')

        ax.legend(loc='upper left', fontsize='small')


def start_plot(server_id, data):
    fig, ax = plt.subplots()
    ani = animation.FuncAnimation(fig, update_plot, fargs=(ax, server_id, data), interval=1000)
    plt.show()


def start(data):
    server_descriptions = [('Server', '192.168.1.80', 61000),
                           ('Client', '192.168.1.80', 61001),
                           ('Proxy', '192.168.1.80', 61060)]
    processes = []

    for i, (description, host, port) in enumerate(server_descriptions):
        process = multiprocessing.Process(target=connect_to_server, args=(i, host, port, data))
        process.start()
        processes.append(process)

    for i in range(3):
        plot_process = multiprocessing.Process(target=start_plot, args=(i, data))
        plot_process.start()
        processes.append(plot_process)

    for process in processes:
        process.join()


if __name__ == "__main__":
    multiprocessing.set_start_method('spawn')
    manager = multiprocessing.Manager()
    data = manager.dict({0: manager.list(), 1: manager.list(), 2: manager.list()})
    start(data)
