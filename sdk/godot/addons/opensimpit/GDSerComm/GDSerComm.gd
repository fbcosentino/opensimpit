extends Node
class_name GDSerComm

signal ports_listed(port_list)

signal data_received(line)

export(int) var DefaultBaudRate = 115200;

export(int) var DefaultTimeout = 1  

enum ByteSizes {
	BYTE_SIZE_8_BITS,
	BYTE_SIZE_7_BITS,
	BYTE_SIZE_6_BITS,
	BYTE_SIZE_5_BITS
}
export(ByteSizes) var ByteSize = ByteSizes.BYTE_SIZE_8_BITS
 
enum ParityTypes {
	PARITY_NONE,
	PARITY_ODD,
	PARITY_EVEN,
	PARITY_MARK,
	PARITY_SPACE
}
export(ParityTypes) var Parity = ParityTypes.PARITY_NONE
 
enum StopBitValues {
	STOPBITS_ONE,
	STOPBITS_ONE_AND_HALF,
	STOPBITS_TWO
}
export(StopBitValues) var StopBits = StopBitValues.STOPBITS_ONE


enum SerialQueues {
	SER_QUEUE_IN,
	SER_QUEUE_OUT,
	SER_QUEUE_ALL
}

onready var mutex = Mutex.new()
onready var semaphore = Semaphore.new()
onready var thread = Thread.new()
var thread_running = false
var thread_should_stop = false

onready var sercomm = preload("res://addons/opensimpit/GDSerComm/SERCOMM.gdns").new()

var is_open: bool = false
var data = PoolByteArray()
#var data: String = ""

var data_queue = []

func _ready():
	yield(get_tree(), "idle_frame") # Wait for all scene tree to be ready
	var __= list_ports() # discarded return value
	
	thread.start(self, "serial_thread")

func list_ports() -> PoolStringArray:
	var port_list = sercomm.list_ports()
	if not port_list is PoolStringArray:
		port_list = PoolStringArray()
	
	emit_signal("ports_listed", port_list)
	
	return port_list


func open_port(port: String, baud: int = DefaultBaudRate, timeout: int = DefaultTimeout):
	if is_open:
		sercomm.close()
	
	var _res = sercomm.open(port, baud, timeout, ByteSize, Parity, StopBits)
	is_open = true
	return is_open

func close_port():
	sercomm.close()
	is_open = false

func send_data(text: String, send_nl_first: bool = true):
	if not is_open:
		print("Error: trying to write to a closed port")
		return
	
	if send_nl_first:
		sercomm.write("\n")
		yield(get_tree().create_timer(0.2), "timeout")
	
	sercomm.write(text)



func serial_thread():
	mutex.lock()
	thread_running = true
	mutex.unlock()
	
	while (true):
		semaphore.wait() # wait for command
		
		mutex.lock()
		var should_stop = thread_should_stop
		mutex.unlock()
		
		if should_stop:
			break
		
		for _i in range(1000):
			mutex.lock()
			should_stop = thread_should_stop
			mutex.unlock()
			
			if should_stop:
				break
			
			if sercomm.get_available() > 0:
#				var c = sercomm.read(0)
#				if c is int:
#					if c > 0:
#						c = char(c)
#					else:
#						print("c: ", c)
#						break
				var c = sercomm.read(1) # int
				if c < 0:
					print("c: ", c)
					break
				
				mutex.lock()
				data.append(c)
#				data += c
				mutex.unlock()
				
		
		mutex.lock()
		if data.size() > 0:
			data_queue.append(data)
			data = PoolByteArray()
		mutex.unlock()
		
	
	sercomm.close()
	mutex.lock()
	thread_running = false
	mutex.unlock()


var serial_timer_count = 0.0
func _physics_process(delta):
	# semaphore countdown
	# semaphore indicates thread must read serial buffer
	if serial_timer_count <= 0:
		serial_timer_count = 0.02
		semaphore.post()
	else:
		serial_timer_count -= delta
	
	# If data was read from serial buffer, emit signal
	mutex.lock()
	var line = data_queue.pop_front()
	mutex.unlock()
	
	if line:
		emit_signal("data_received", line)


func _exit_tree():
	mutex.lock()
	thread_should_stop = true
	mutex.unlock()
	yield(get_tree(), "idle_frame")
	thread.wait_to_finish()
	print("Serial port thread running: ", thread_running)
