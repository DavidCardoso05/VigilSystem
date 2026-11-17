> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `claude/VigilSystem – Class Diagrams for Slides (PlantUML).md` · Document date: 4 Nov 2026 · Superseded by: Master Context §E.3 (diagrams D12–D14 to be redrawn in draw.io).
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – Class Diagrams for Slides (PlantUML, 3 diagrams)

Rendered with PlantUML 1.2024.7 using the ELK layout engine (`!pragma layout elk`). Orthogonal routing; no connector passes through a class box.

## Diagram 1

```plantuml
@startuml d1
!pragma layout elk
skinparam linetype ortho
skinparam classAttributeIconSize 0
hide empty members
hide circle
skinparam shadowing false
skinparam backgroundColor #FFFFFF
skinparam defaultFontName Arial
skinparam defaultFontSize 15
skinparam class {
  BorderColor #1F3A5F
  BackgroundColor #FFFFFF
  ArrowColor #1F3A5F
  FontStyle bold
  StereotypeFontSize 11
}
skinparam package {
  BorderColor #1F3A5F
  FontColor #1F3A5F
  FontStyle bold
  Style frame
}
skinparam note {
  BorderColor #8A8A8A
  BackgroundColor #FFFBE6
}
skinparam arrowFontSize 11
scale max 2048*1536
title Class Diagram — Overview (by package)

package threads #EAF1FB {
  class SupervisorThread <<pthread>>
  abstract class WorkerThread <<abstract>>
  class LocalInputThread
  class IRSensorsThread
  class MQ2SensorThread
  class PowerMonitorThread
  class AlarmControllerThread
  class OutputThread
  class StorageThread
  class NetworkMQTTThread
}
package core #EEF7EE {
  class EventQueue
  class Event <<struct>>
  class "CommandQueue<T>" as CommandQueue
  class OutputCmd <<struct>>
  class StorageCmd <<struct>>
  class NetCmd <<struct>>
  class PiMutex
}
package logic #F5EEF8 {
  class AlarmLogic
  class PinHasher
}
package hw #FDF2E6 {
  interface IGpioInput <<interface>>
  interface IGpioOutput <<interface>>
  interface IPwm <<interface>>
  interface IPowerSensor <<interface>>
  class GpioLines
  class PwmDevice
  class Ina219
  class Watchdog
}
package storage #F2F2F2 {
  interface IEventSink <<interface>>
  class Database
}
package net #E8F6F8 {
  class MqttClient
  class LinkMonitor
  class Protocol
}

' supervision + generalisation (points up)
SupervisorThread o-- "8" WorkerThread : heartbeat / restart
WorkerThread <|-- LocalInputThread
WorkerThread <|-- IRSensorsThread
WorkerThread <|-- MQ2SensorThread
WorkerThread <|-- PowerMonitorThread
WorkerThread <|-- AlarmControllerThread
WorkerThread <|-- OutputThread
WorkerThread <|-- StorageThread
WorkerThread <|-- NetworkMQTTThread

' event / command queues
LocalInputThread ..> EventQueue : push
IRSensorsThread ..> EventQueue : push
MQ2SensorThread ..> EventQueue : push
PowerMonitorThread ..> EventQueue : push
NetworkMQTTThread ..> EventQueue : push
AlarmControllerThread --> "1" EventQueue : consumes
AlarmControllerThread --> "3" CommandQueue : OutputCmd / StorageCmd / NetCmd
AlarmControllerThread *-down- "1" AlarmLogic

' compositions down to hw / services
LocalInputThread *-down- GpioLines
IRSensorsThread *-down- GpioLines
MQ2SensorThread *-down- GpioLines
OutputThread *-down- GpioLines
OutputThread *-down- PwmDevice
PowerMonitorThread *-down- Ina219
SupervisorThread *-down- Watchdog
StorageThread *-down- Database
NetworkMQTTThread *-down- MqttClient
NetworkMQTTThread *-down- LinkMonitor

' realisations (triangle on the interface)
GpioLines .up.|> IGpioInput
GpioLines .up.|> IGpioOutput
PwmDevice .up.|> IPwm
Ina219 .up.|> IPowerSensor
NetworkMQTTThread .down.|> IEventSink
StorageThread -down-> IEventSink

' layout helpers: logic / storage / net in a lower band
PiMutex -[hidden]down- PinHasher
PiMutex -[hidden]down- Database
PiMutex -[hidden]down- Protocol
@enduml
```

## Diagram 2

```plantuml
@startuml d2
!pragma layout elk
skinparam linetype ortho
skinparam classAttributeIconSize 0
hide empty members
hide circle
skinparam shadowing false
skinparam backgroundColor #FFFFFF
skinparam defaultFontName Arial
skinparam class {
  BorderColor #1F3A5F
  BackgroundColor #FFFFFF
  ArrowColor #1F3A5F
  FontStyle bold
  StereotypeFontSize 11
}
skinparam package {
  BorderColor #1F3A5F
  FontColor #1F3A5F
  FontStyle bold
  Style frame
}
skinparam note {
  BorderColor #8A8A8A
  BackgroundColor #FFFBE6
}
skinparam arrowFontSize 11
scale max 2048*1536
skinparam defaultFontSize 13
title Class Diagram — Threads, queues and logic (complete)

package threads #EAF1FB {
  abstract class WorkerThread <<abstract>> <<pthread>> {
    #tid: pthread_t
    #running: atomic<bool>
    +start(): void
    +stop(): void
    +lastHeartbeatNs(): uint64_t
    {abstract} #run(): void
    #wake(): void
    #heartbeat(): void
  }
  class SupervisorThread <<pthread>> {
    -wdt: Watchdog
    +onHeartbeat(threadId: int): void
    -restartThread(threadId: int): void
    -feedWatchdog(): void
  }
  class LocalInputThread {
    -pinBuffer: std::string
    -scanPeriodMs: int = 15
  }
  class IRSensorsThread {
    -lastState: bool[2]
    -retriggerGuardMs: int = 200
  }
  class MQ2SensorThread {
    -warmUpS: int
    -requiredReads: int
    -smokePresent: std::atomic<bool>
    +isSmokePresent(): bool
  }
  class PowerMonitorThread {
    -pollPeriodMs: int = 1000
    -hysteresisSamples: int = 3
  }
  class AlarmControllerThread {
    -opState: OperationalState
    -fireState: FireState
    -powerState: PowerState
    -netState: NetState
    -epfd: int
    -exitTimerFd: int {60 s}
    -entryTimerFd: int {30 s}
    -silenceTimerFd: int {10 min}
    -lockoutTimerFd: int {5 min}
    -wrongPinCount: int
    -pinHash: std::string
    -pendingPinChange: optional<PendingPin>
    -activeFaults: set<FaultComponent>
    -entrySensor: int
    -batteryPct: int
    -handleEvent(e: Event): void
    -onTimerExpired(fd: int): void
    -verifyPin(pin: string): bool
    -handleRemoteArm(): void
    -handleRemoteDisarm(pin: string): void
    -handleChangePin(pin: string, newPin: string): void
  }
  class OutputThread {
    -cmdQueue: CommandQueue<OutputCmd>
  }
  class StorageThread {
    -cmdQueue: CommandQueue<StorageCmd>
    +logEvent(cmd: StorageCmd): int
    +loadPinHash(): string
    +savePinHash(hash: string): bool
    +getPendingAlerts(limit: int): vector<Event>
    +markSent(eventId: int): void
    +getEventsSince(eventId: int): vector<Event> {max 20}
  }
  class NetworkMQTTThread {
    -cmdQueue: CommandQueue<NetCmd>
    -backoffS: int {1 s → 60 s}
    -maxResendPerS: int = 5
    +onEventCommitted(eventId: int, isAlert: bool): void
    +publishAlert(e: Event): void
    +publishStatus(): void
    -resendPending(): void
  }
}

package core #EEF7EE {
  class Event <<struct>> {
    +type: EventType
    +sourceId: int
    +timestamp: time_t
    +payload: std::string
    +monoNs: uint64_t
  }
  class EventQueue {
    -queue: std::queue<Event>
    -mtx: PiMutex
    -cv: condition_variable_any
    -efd: int {eventfd}
    +push(e: Event): void
    +tryPop(out e: Event): bool
    +getFd(): int
  }
  class CommandQueue<T> {
    -queue: std::queue<T>
    -mtx: PiMutex
    -cv: condition_variable_any
    +push(cmd: T): void
    +pop(): T
    +popFor(timeout): optional<T>
    +close(): void
  }
  class PiMutex {
    +lock(): void
    +unlock(): void
  }
  class OutputCmd <<struct>> {
    +kind: {SpeakerBase, SpeakerBeep, RedLed, RedLedBlink, Rgb}
    +speaker: SpeakerPattern
    +led: RedLedMode
    +colour: RgbColour
    +faultBlink: bool
  }
  class StorageCmd <<struct>> {
    +kind: {LogEvent, SavePinHash, Backup, CloseForShutdown}
    +eventType: string
    +details: optional<string>
    +isAlert: bool
    +timestamp: time_t
    +thenShutdown: bool
    +pinHash: string
    +requestId: string
  }
  class NetCmd <<struct>> {
    +kind: {LoggedEvent, Status, Response, Connected,
    \tConnectionLost, MessageArrived, Delivered}
    +json: string
    +eventId: int
    +isAlert: bool
    +thenShutdown: bool
  }
}

package storage #F2F2F2 {
  interface IEventSink <<interface>>
}

package logic #F5EEF8 {
  class AlarmLogic {
    +handle(e: Event, now): vector<Action>
  }
  class PinHasher {
    +hash(pin: string): string
    +verify(pin: string, stored: string): bool
  }
}


' --- enumerations: owned by core (EventType, SpeakerPattern, RedLedMode, RgbColour)
'     and logic (OperationalState, FireState, PowerState, NetState, FaultComponent);
'     drawn outside the frames so they sit in one row at the bottom
enum EventType <<enumeration>> {
  .. pushed through EventQueue ..
  EVT_KEY_DIGIT
  EVT_PIN_SUBMIT
  EVT_BUTTON_PRESS
  EVT_MOTION_DETECTED
  EVT_SMOKE_DETECTED
  EVT_POWER_LOST
  EVT_POWER_RESTORED
  EVT_BATTERY_LOW
  EVT_BATTERY_CRITICAL
  EVT_BATTERY_LEVEL
  EVT_NET_DISCONNECTED
  EVT_NET_RESTORED
  EVT_REMOTE_ARM
  EVT_REMOTE_DISARM
  EVT_REMOTE_CHANGE_PIN
  EVT_FAULT
  EVT_PIN_SAVE_RESULT
  .. raised inside AlarmControllerThread ..
  EVT_SYSTEM_STARTUP
  EVT_PIN_CORRECT
  EVT_PIN_WRONG
  EVT_EXIT_DELAY_TIMEOUT
  EVT_ENTRY_DELAY_TIMEOUT
  EVT_SMOKE_SILENCE_TIMEOUT
  EVT_LOCKOUT_TIMEOUT
}
enum OperationalState <<enumeration>> {
  DISARMED
  EXIT_DELAY
  ARMED
  ENTRY_DELAY
  INTRUSION_ALARM
}
enum FireState <<enumeration>> {
  NORMAL
  FIRE_ALARM
  SILENCED
}
enum PowerState <<enumeration>> {
  AC_POWERED
  BATTERY_MODE
  LOW_BATTERY
  SHUTDOWN
}
enum NetState <<enumeration>> {
  NET_CONNECTED
  NET_LOST
}
enum FaultComponent <<enumeration>> {
  IR1
  IR2
  MQ2
  SPEAKER
  PWM_DRIVER
  DATABASE
}
enum SpeakerPattern <<enumeration>> {
  OFF
  KEY_BEEP
  ERROR_BEEP
  WARNING
  INTRUSION
  FIRE
}
enum RedLedMode <<enumeration>> {
  OFF
  ON
  FLASH
}
enum RgbColour <<enumeration>> {
  OFF
  GREEN
  YELLOW
  BLUE
  MAGENTA
}

note bottom of AlarmLogic : pure: no threads, no I/O;\nthe 4 FSMs
note bottom of PinHasher : PBKDF2-HMAC-SHA256
note bottom of PiMutex : PTHREAD_PRIO_INHERIT
note "hardware/service classes:\nsee next slide" as NHW

' --- generalisation (points up to WorkerThread) + supervision
WorkerThread <|-- LocalInputThread
WorkerThread <|-- IRSensorsThread
WorkerThread <|-- PowerMonitorThread
WorkerThread <|-- AlarmControllerThread
WorkerThread <|-- MQ2SensorThread
WorkerThread <|-- OutputThread
WorkerThread <|-- NetworkMQTTThread
WorkerThread <|-- StorageThread
WorkerThread "8" --o "1" SupervisorThread : heartbeat / restart

' --- controller -> consumer threads (one CommandQueue<T> each)
AlarmControllerThread ..> MQ2SensorThread : isSmokePresent()
AlarmControllerThread --> "1" OutputThread : OutputCmd
AlarmControllerThread --> "1" NetworkMQTTThread : NetCmd
AlarmControllerThread --> "1" StorageThread : StorageCmd
NetworkMQTTThread ..> StorageThread : outbox, get_events
AlarmControllerThread *-- "1" AlarmLogic
AlarmControllerThread ..> PinHasher : verify

' --- IEventSink (storage package; detailed on next slide)
NetworkMQTTThread ..|> IEventSink
StorageThread --> IEventSink : after COMMIT

' --- producers push into the single EventQueue
LocalInputThread ..> EventQueue : push
IRSensorsThread ..> EventQueue : push
MQ2SensorThread ..> EventQueue : push
PowerMonitorThread ..> EventQueue : push
OutputThread ..> EventQueue : push
NetworkMQTTThread ..> EventQueue : push
StorageThread ..> EventQueue : push
AlarmControllerThread --> "1" EventQueue : consumes (epoll on eventfd)
EventQueue "1" o-- "0..*" Event

' --- layout helpers (hidden): queues+logic band, structs band, enumerations row at the bottom
StorageThread -[hidden]- CommandQueue
StorageThread -[hidden]- PiMutex
StorageThread -[hidden]- AlarmLogic
StorageThread -[hidden]- PinHasher
EventQueue -[hidden]- OutputCmd
EventQueue -[hidden]- StorageCmd
EventQueue -[hidden]- NetCmd
StorageCmd -[hidden]- EventType
StorageCmd -[hidden]- OperationalState
StorageCmd -[hidden]- FireState
StorageCmd -[hidden]- PowerState
StorageCmd -[hidden]- NetState
StorageCmd -[hidden]- FaultComponent
StorageCmd -[hidden]- SpeakerPattern
StorageCmd -[hidden]- RedLedMode
StorageCmd -[hidden]- RgbColour
@enduml
```

## Diagram 3

```plantuml
@startuml d3
!pragma layout elk
skinparam linetype ortho
skinparam classAttributeIconSize 0
hide empty members
hide circle
skinparam shadowing false
skinparam backgroundColor #FFFFFF
skinparam defaultFontName Arial
skinparam class {
  BorderColor #1F3A5F
  BackgroundColor #FFFFFF
  ArrowColor #1F3A5F
  FontStyle bold
  StereotypeFontSize 11
}
skinparam package {
  BorderColor #1F3A5F
  FontColor #1F3A5F
  FontStyle bold
  Style frame
}
skinparam note {
  BorderColor #8A8A8A
  BackgroundColor #FFFBE6
}
skinparam arrowFontSize 11
scale max 2048*1536
skinparam defaultFontSize 13
title Class Diagram — Hardware and service classes (complete)

package threads #EAF1FB {
  class LocalInputThread
  class IRSensorsThread
  class MQ2SensorThread
  class OutputThread
  class PowerMonitorThread
  class SupervisorThread <<pthread>>
  class StorageThread
  class NetworkMQTTThread
}

package hw #FDF2E6 {
  interface IGpioInput <<interface>> {
    +get(offset: int): int
    +waitEdge(timeoutNs: int64): bool
  }
  interface IGpioOutput <<interface>> {
    +set(offset: int, value: int): void
  }
  interface IPwm <<interface>> {
    +writeCommand(channel, patternId, frequencyHz,
    \tdutyPercent, enable): int
  }
  interface IPowerSensor <<interface>> {
    +readBusVoltage(): float
    +readCurrent(): float
  }
  class GpioLines {
    -chip: GpioChipHandle
    -request: GpioRequestHandle
    +requestLines(offsets: vector<int>, dir: int, bias: int): bool
    +get(offset: int): int
    +set(offset: int, value: int): void
    +waitEdge(timeoutNs: int64): bool
    +readEdges(): vector<Edge>
  }
  class PwmDevice {
    -fd: int {/dev/vigil_pwm}
    -mtx: PiMutex
    +writeCommand(channel: uint8, patternId: uint8,
    \tfrequencyHz: uint32, dutyPercent: uint8, enable: bool): int
  }
  class Ina219 {
    -fd: int {/dev/i2c-1}
    -address: uint8 = 0x42
    +readBusVoltage(): float
    +readCurrent(): float
  }
  class Watchdog {
    -fd: int {/dev/watchdog}
    +setTimeout(s: int): void
    +keepAlive(): void
    +disable(): void
  }
}

package storage #F2F2F2 {
  interface IEventSink <<interface>> {
    +onEventCommitted(eventId: int, isAlert: bool): void
  }
  class Database {
    -db: sqlite3*
    -mtx: PiMutex
    +open(path: string): bool
    +exec(sql: string): bool
    +prepare(sql: string): sqlite3_stmt*
  }
}

package net #E8F6F8 {
  class MqttClient {
    -client: mqtt::async_client
    +connect(): bool
    +publish(topic: string, payload: string, qos: int, retain: bool): void
    +subscribe(topic: string, qos: int): void
    +disconnect(): void
  }
  class LinkMonitor {
    -nlSocket: int {netlink}
    +poll(): optional<bool>
  }
  class Protocol {
    +buildAlert(e: Event): string
    +buildStatus(...): string
    +parseCommand(json: string): optional<Command>
  }
}

note bottom of GpioLines : opaque handles,\nimplemented with libgpiod v1
note bottom of PwmDevice : client of our custom\nkernel driver vigil_pwm
note bottom of Database : embedded library,\nin-process
note bottom of LinkMonitor : Wi-Fi link up/down ≤ 2 s
note "Real classes on the Pi; simulated classes\non the laptop implement the same interfaces." as NDIP

' --- dependency inversion: threads depend on the interfaces
LocalInputThread ..> IGpioInput : uses
LocalInputThread ..> IGpioOutput : uses
IRSensorsThread ..> IGpioInput : uses
MQ2SensorThread ..> IGpioInput : uses
OutputThread ..> IGpioOutput : uses
OutputThread ..> IPwm : uses
PowerMonitorThread ..> IPowerSensor : uses

' --- realisations (triangle on the interface, pointing up)
IGpioInput <|.. GpioLines
IGpioOutput <|.. GpioLines
IPwm <|.. PwmDevice
IPowerSensor <|.. Ina219

' --- compositions (owner on top, part below)
LocalInputThread *-- "2" GpioLines : rows+button / columns
IRSensorsThread *-- "1" GpioLines : GPIO16, GPIO20
MQ2SensorThread *-- "1" GpioLines : GPIO21
OutputThread *-- "1" GpioLines : RGB (3 lines)
OutputThread *-- "1" PwmDevice : speaker, red LED
PowerMonitorThread *-- "1" Ina219
SupervisorThread *-- "1" Watchdog
StorageThread *-- "1" Database
NetworkMQTTThread *-- "1" MqttClient
NetworkMQTTThread *-- "1" LinkMonitor
NetworkMQTTThread ..> Protocol : uses

' --- event sink (after-COMMIT callback)
NetworkMQTTThread ..|> IEventSink
StorageThread --> "1" IEventSink : after COMMIT

' --- layout helpers (hidden): net band below storage
IEventSink -[hidden]- Protocol
IEventSink -[hidden]- MqttClient
IEventSink -[hidden]- LinkMonitor
@enduml
```
