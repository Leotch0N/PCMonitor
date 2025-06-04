from flask import Flask, jsonify
import time
import random
import win32pdh


app = Flask(__name__)

counter__cpu_time = r'\Processor(_Total)\% Processor Time'
counter__cpu_utility = r'\Processor Information(_Total)\% Processor Utility'

counter__memory_available = r'\Memory\Available MBytes'
counter__memory_committed = r'\Memory\Committed Bytes'
counter__memory_cache = r'\Memory\Cache Bytes'

counter__disk_read =  r"\PhysicalDisk(_Total)\Disk Read Bytes/sec"
counter__disk_write =  r"\PhysicalDisk(_Total)\Disk Write Bytes/sec"

# counter__network_received = r'\Network Interface(Intel[R] Wi-Fi 6 AX200 160MHz)\Bytes Received/sec'
# counter__network_sent = r'\Network Interface(Intel[R] Wi-Fi 6 AX200 160MHz)\Bytes Sent/sec'
counter__network_received = r'\Network Interface(Intel[R] Ethernet Controller [3] I225-V _2)\Bytes Received/sec'
counter__network_sent = r'\Network Interface(Intel[R] Ethernet Controller [3] I225-V _2)\Bytes Sent/sec'


query = win32pdh.OpenQuery()

counter_handle__cpu_utility = win32pdh.AddCounter(query, counter__cpu_utility)
counter_handle__memory_available = win32pdh.AddCounter(query, counter__memory_available)
counter_handle__disk_read = win32pdh.AddCounter(query, counter__disk_read)
counter_handle__disk_write = win32pdh.AddCounter(query, counter__disk_write)
counter_handle__network_received = win32pdh.AddCounter(query, counter__network_received)
counter_handle__network_sent = win32pdh.AddCounter(query, counter__network_sent)

win32pdh.CollectQueryData(query)
time.sleep(1)

counters = {
    "CPU": counter_handle__cpu_utility,
    "RAM": counter_handle__memory_available,
    # "DISK READ": counter_handle__disk_read,
    # "DISK WRITE": counter_handle__disk_write,
    "RECEIVED":counter_handle__network_received,
    "SENT":counter_handle__network_sent
}


# 用于存储监控数据的数组
monitoring_data = []

def collect_monitoring_data():
    """模拟收集监控数据的函数"""
    while True:
        # 监控数据（例如：网络速度、CPU 使用率等）
        data_point = {}
        for name, counter_handle in counters.items():
            win32pdh.CollectQueryData(query)
            try:
                _, value = win32pdh.GetFormattedCounterValue(counter_handle, win32pdh.PDH_FMT_DOUBLE)
            except Exception:
                value =0
                print("Exception with: ", name, counter_handle)
            # print(f"{name}: {value}")
            data_point[name] = int(value) if name in {'CPU', 'RAM'} else int(value / 1024)
        # data_point['timestamp'] = time.time()
        
        # 将数据点添加到数组
        monitoring_data.append(data_point)
        
        # 限制数组长度
        if len(monitoring_data) > 5:
            monitoring_data.pop(0)
        
        time.sleep(2)  # 每2秒收集一次数据


@app.route('/monitoring-data', methods=['GET'])
def get_monitoring_data():
    """HTTP 接口，返回监控数据"""
    return jsonify(monitoring_data)


if __name__ == '__main__':
    # 启动监控数据收集线程
    import threading
    threading.Thread(target=collect_monitoring_data, daemon=True).start()
    
    # 启动 Flask Web 服务器
    app.run(host='0.0.0.0', port=5000)
