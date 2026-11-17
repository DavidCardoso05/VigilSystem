> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `claude/VigilSystem – Presentation Changes (DEC-001 to DEC-006).md` · Document date: 9 Nov 2026 · Superseded by: `docs/presentation/DESIGN_DECK_CHANGES.md` v1.0.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – Alterações à Apresentação de Design (DEC-001 a DEC-010)

Grupo 8 · David Cardoso (PG63873) · Ana Sofia Cunha (A107017) · Criado a 9 nov 2026 · Atualizado a 9 nov 2026 (DEC-010)

## 1. Âmbito deste documento

- **O que é.** A lista do que tem de mudar no deck atual (`G8_VigilSystem_D_Presentation.pptx`, 88 slides) por causa das decisões aprovadas até agora: DEC-001, DEC-002, DEC-003a, DEC-003b, DEC-004, DEC-005a, DEC-005b, DEC-006, DEC-007, DEC-008, DEC-009 e DEC-010. As decisões e as suas razões estão em `claude/VigilSystem – Simplification Decision Log.md`.
- **O que não é.** Não é a nova estrutura da apresentação. A reestruturação (narrativa, cenários, anexo, número final de slides) é a Fase 4 e ainda não começou: depende das informações e aprovações listadas na secção 6. As linhas que dependem dessas decisões estão marcadas.
- **Nada foi alterado no deck.** Executar estas alterações exige autorização explícita.
- **Nome do ficheiro.** Mantém "(DEC-001 to DEC-006)" até haver autorização para o mudar; o conteúdo cobre até à DEC-010.
- **Ferramenta dos diagramas.** Ainda não está decidida (a regra do draw.io faz parte do ficheiro de diretrizes, ainda por aprovar).
- **Idioma.** Os slides continuam em inglês, por isso os textos a colocar nos slides estão em inglês.
- **Verificação.** Os números de slide foram confirmados contra o texto e as imagens do deck de 88 slides. As linhas marcadas com "(confirmar na execução)" referem conteúdo que não foi possível verificar em detalhe.

## 2. Alterações transversais (procurar em todo o deck)

| Procurar | Substituir por | Decisão |
|---|---|---|
| `IRSensorsThread`, `MQ2SensorThread` | `SafetySensorsThread` | DEC-003a |
| `OutputThread` | `AlarmControllerThread` (classe `AlarmOutputs`) | DEC-003a/b |
| `StorageThread`, `NetworkMQTTThread` | `CommThread` (classes `EventStore`, `MqttLink`, `OutboxPublisher`, `Protocol`) | DEC-003a/b |
| `SupervisorThread` | `main` (classes `HealthMonitor` + `Watchdog`) | DEC-003a |
| `OutputCmd`, `StorageCmd`, `NetCmd`, três `CommandQueue<…>` | `CommCommand` numa só `CommQueue`; as saídas são aplicadas diretamente pelo controller | DEC-003a/b |
| `IEventSink`, "committed event → Network" | Remover | DEC-003a |
| `LinkMonitor`, netlink, "Wi-Fi loss ≤ 2 s", "wpa_supplicant event" (como deteção) | Remover; "broker loss detected by keep-alive (≤ 60 s)" | DEC-001 |
| epoll, eventfd, timerfd, "4 timerfds" | "waits on the event queue until the next deadline (pthread_cond, CLOCK_MONOTONIC)" | DEC-003a |
| `isSmokePresent()` | Eventos `SMOKE_DETECTED` / `SMOKE_CLEARED` | DEC-003a |
| `EVT_BATTERY_LEVEL`, battery %, `battery_pct`, "10 % steps" | Remover | DEC-001 (M1) |
| "≤ 30 %" (bateria fraca) | "≤ 6.7 V" | DEC-005a |
| "shutdown", "power off" (bateria crítica) | "safe state" | DEC-005b |
| `PENDING_NOTIFICATION`, "outbox row" | `EVENT_LOG.status` / `sent_at`, "event stays PENDING" | DEC-006 |
| `get_events`, "App sync", "recovered by get_events" | Remover | DEC-001 (M1) |
| "auth errors every 5 min", "Retry every 5 min" | "backoff 1 s → 60 s for all failures; logged once per episode" | DEC-006 |
| `PWM_DRIVER` (FaultComponent) | `SPEAKER` | DEC-004 |
| "Retry GPIO request every 10 s", "re-request the line every 10 s" | "FAULT once; component ignored until reboot" | DEC-004 |
| RGB "fault blink", "blinking at 1 Hz" | Remover | DEC-001 |
| "Supervisor restarts it", "hung thread is restarted" | "watchdog resets the Pi; last ARMED/DISARMED state restored at boot" | DEC-001, DEC-004 |
| "Device Offline (clean)" com motivo, "DISCONNECT 0x00" | "planned stop: DISCONNECT 0x04 (broker publishes the Last Will)" | DEC-005b |
| "starts DISARMED" | "restores the last ARMED/DISARMED state" | DEC-004 |
| `pattern_id`, 8 padrões no driver, "base / one-shot" | "step sequences supplied by user space" | DEC-002 |
| "duplicates dropped by event_id" | "exact duplicates dropped; list in arrival order" | DEC-006 |
| IR "NPN open-collector (to be confirmed by measurement) → 3.3 V pull-up on GPIO" | "Schottky diode in series + external 10 kΩ pull-up to 3.3 V (safe for open-collector or 5 V output)" | DEC-007 |
| "object in front of IR", "object in its detecting scope" | "the door/window monitored by IR1 moves (closed → open)" | DEC-010 |
| "PIR", "HC-SR501" (se aparecer como sensor de intrusão) | "IR reflective sensor on the door/window frame" | DEC-010 |
| RGB "GPIO LOW = on", "active-low (sink)" (se aparecer) | "NPN low-side per cathode; GPIO HIGH = on" (os slides 11–12 já usam este circuito) | DEC-007 |

## 3. Nunca apresentar (M1 e itens removidos)

Antes de apresentar, procurar no deck e confirmar que nada disto aparece:

- percentagem de bateria e o campo `battery_pct`;
- alertas de energia no arranque (POWER_RESTORED "BOOT", POWER_LOST ao arrancar a bateria);
- backup e restauro da base de dados;
- `get_events` e sincronização de histórico;
- resumo de reconexão na app ("N alerts received");
- deteção de perda de Wi-Fi em ≤ 2 s;
- reinício de threads pelo Supervisor;
- pisca de falha do LED RGB;
- `poweroff` por bateria crítica;
- equipamento de medição ou testes "no laboratório" (DEC-008, 8.3: o método de medição fica só como nota);
- onde o SOLID não se aplica de propósito (classes sem interface, FSMs sem State pattern) e a afirmação "new event = EventType value" (DEC-009).

## 4. Alterações slide a slide

| Slide | Título atual | O que muda | Decisão |
|---|---|---|---|
| 1 | Cover | — | — |
| 2 | Agenda | Atualizar no fim, quando a estrutura da Fase 4 estiver fechada | Fase 4 |
| 3 | Problem Statement | Sem alterações (continua válido) | — |
| 4–8 | System overview, Hardware Architecture, Hardware Specification (Pi, UPS HAT, baterias) | Sem alterações por decisões | — |
| 9 | Hardware Specification (MQ-2, IR) | **IR, Output:** "NPN open-collector (to be confirmed by measurement) → 3.3 V pull-up on GPIO" → "Open-collector or 5 V output (to be measured). Interface: Schottky diode (BAT85) in series + external 10 kΩ pull-up to 3.3 V, safe for both. Door closed = LOW, open = HIGH". "A low voltage output will be given when an object is in its detecting scope" → "Mounted on the door/window frame: LOW while the leaf is in front (closed), HIGH when it moves away (open)". Pode levar aqui a linha de justificação da DEC-010 (secção 5). MQ-2 sem alterações | 007, 010 |
| 10 | Hardware Specification (Speaker, Keypad) | Sem alterações por decisões | — |
| 11 | Hardware Specification (Push Button, RGB LED, Red LED) | **RGB:** acrescentar "Anode at +5 V; 10 kΩ base pull-down keeps the LED off during boot". A linha "Each cathode switched by an NPN (BC337, 1 kΩ base) — GPIO logic HIGH = ON" já está correta e mantém-se | 007 |
| 12 | Hardware Connections | **IR1 e IR2:** acrescentar um Schottky (BAT85) entre OUT e o GPIO, com o cátodo do lado do sensor, e passar o pull-up de 10 kΩ para o lado do GPIO. O RGB (NPN, 1 kΩ, 10 kΩ) já está correto. Opcional: um contorno com a legenda "Interface board (protoboard)" à volta dos circuitos de apoio | 007 |
| 13 | Hardware Layout | As imagens mantêm-se (já mostram a caixa, a placa de interface e os sensores remotos). Acrescentar uma legenda curta: "Bring-up on breadboard → interface board on protoboard → enclosure. IR on the door/window frame; MQ-2 high on the wall or ceiling". A caixa é uma caixa de plástico comercial com furos (decidido a 9 nov). Se as imagens não corresponderem à caixa comprada, substituí-las na Fase 4 por uma foto ou um esquema da montagem real | 007 |
| 14 | GPIO Allocation | **GPIO 16 / 20:** "Input pin for IR sensor 1 / 2 (door/window): external 10 kΩ pull-up + Schottky; LOW = closed". **GPIO 4 / 26 / 19:** "Output pin to drive the NPN of the RGB LED red / green / blue channel (HIGH = on)" | 007 |
| 15 | Power Budget | Sem alterações (a corrente do RGB já sai do 5 V; as bases somam ≈ 8 mA, desprezável) | — |
| 16 | IR Sensor Module Interface | **Responsible thread:** `SafetySensorsThread`. **Activation:** "Periodic 100 ms sampling". **Detection:** "Closed → open transition confirmed by 2 consecutive reads (one event per opening)"; remover "Any edge", "Blocking edge wait, 1 s timeout" e "Retrigger guard 200 ms". **Initialization:** "First reading is the baseline (no event at boot)". **Fault:** "FAULT(IR1/IR2) once; sensor ignored until reboot"; remover "Retry GPIO request every 10 s". **Event generated:** `EVT_MOTION_DETECTED(sourceId)` mantém o nome (ligação à Análise), com a descrição "door/window movement (closed → open)". **Limitations:** "only the monitored doors/windows; armed with a door open → detected after it closes and reopens" | 003a, 004, 010 |
| 17 | MQ-2 Sensor Module Interface | **Responsible thread:** `SafetySensorsThread`. **Event generated:** "SMOKE_DETECTED on clear → present; SMOKE_CLEARED on present → clear"; remover a linha "Current smoke state: isSmokePresent()". **Fault/recovery:** "FAULT(MQ2) once; ignored until reboot" (remover a nova tentativa a cada 10 s) | 003a, 004 |
| 18 | Keypad Interface | "PIN validation is performed by the AlarmControllerThread" → "by AlarmLogic (PinGuard)". Acrescentar a linha "Fault: FAULT(KEYPAD) once; remote disarm still available" | 003b, 004 |
| 19 | Push-Button Interface | "State validation is performed by the AlarmControllerThread" → "by AlarmLogic (IntrusionFsm)". O debounce mantém-se (2 leituras a 15 ms). Falha coberta por FAULT(KEYPAD) | 003b, 004 |
| 20 | RGB LED Interface | **Responsible thread:** `AlarmControllerThread (AlarmOutputs)`. **Activation:** "Set directly by the controller (no command queue)". Remover as linhas "Device fault", "Fault blink" e "Fault blink priority". A linha do estado inicial mantém-se (a ligação começa como perdida até ao primeiro CONNACK) | 001, 003a/b |
| 21 | Red LED Interface | **Responsible thread:** `AlarmControllerThread (AlarmOutputs)`. Os padrões passam a ser sequências enviadas para `/dev/vigil_pwm`. **Activation:** direta, sem fila. "EVT_FAULT(PWM_DRIVER)" → "FAULT(SPEAKER)". "Driver failure: RGB fault blink is the fallback indicator" → "FAULT alert; RGB keeps showing power and network" | 001, 002, 003a, 004 |
| 22 | Speaker Interface | **Responsible thread:** `AlarmControllerThread (AlarmOutputs)`. Os tons passam a ser definidos em user space e enviados como sequências de passos. "Beeps during entry warning: warning resumes" mantém-se (o user space repõe o aviso com um prazo). **Activation:** direta. "Fault split" → um só "FAULT(SPEAKER)" | 002, 003a, 004 |
| 23 | UPS / Power Monitor Interface | Classes `PowerMonitorThread` + `PowerSupplyMonitor`. Remover "Battery percentage", "Battery status reporting" e `EVT_BATTERY_LEVEL`. **Low battery:** "≤ 6.7 V for 3 samples, on battery, once per outage". **Critical:** "≤ 6.4 V for 3 samples → safe state". **Payloads:** POWER_LOST sem payload; BATTERY_LOW = tensão. Acrescentar "Initial state: decided by the first 3 samples, no alert". **I²C failure:** "FAULT(POWER_MONITOR) after consecutive failures" | 001, 004, 005a, 005b |
| 24–25 | Software Tools, Software COTS | Sem alterações por decisões | — |
| 26 | Data Storage Specification | **Redesenhar.** A outbox passa a ser as colunas `status`/`sent_at` do EVENT_LOG. Todos os eventos (alertas e registo): INSERT PENDING → publish → SENT no PUBACK. Remover PENDING_NOTIFICATION e as leituras de `get_events`. Dono: `CommThread` (`EventStore`, `OutboxPublisher`). Falha da base de dados no arranque: base nova + PIN por omissão + FAULT(DATABASE) + DISARMED. Em funcionamento: só syslog. Sem backup | 001, 003b, 004, 006 |
| 27 | Database ERD | **Redesenhar com 2 tabelas:** EVENT_LOG (`id`, `event_type`, `timestamp`, `details`, `status`, `sent_at`) e PIN_CONFIG. Remover PENDING_NOTIFICATION e a relação 1 : 0..1. Acrescentar uma nota a justificar a mudança face à Análise (de 3 para 2 tabelas) | 006 |
| 28 | Cloud Architecture | Sem alterações | — |
| 29 | Message Specification – Remote Commands | Remover o comando "Get Events" e a sua resposta. O resto mantém-se | 001 |
| 30 | Message Specification – System Status & Connections | Remover `battery_pct` e a nota "only in 10 % steps". Remover a mensagem "Device Offline (clean)" com `reason` e o caminho "WillDiscarded / Clean DISCONNECT 0x00". Acrescentar "Planned stop: DISCONNECT 0x04 → broker publishes the Last Will immediately". Mantêm-se "Device Online" e o Last Will | 001, 005b |
| 31 | Individual Real-Time Alerts (Intrusion, Fire, Fire silenced, Hazard cleared) | Nas caixas PUBACK/No PUBACK: "row" → "event" (o evento fica PENDING no EVENT_LOG). O conteúdo das mensagens não muda | 006 |
| 32 | Individual Real-Time Alerts (Power Lost, Power Restored, Low Battery, Shutdown) | **Power Lost:** `details` null. **Power Restored:** remover o gatilho "or boot on mains after a SHUTDOWN"; `details` null. **Low Battery:** gatilho "on battery, ≤ 6.7 V for 3 samples"; `details` = tensão (ex.: "6.68 V"). **Shutdown:** gatilho "≤ 6.4 V for 3 samples → safe state (DB closed, sync; keeps running until the HAT cuts power)"; remover "Waits up to 5 s for PUBACK, then Device Offline (clean), DISCONNECT 0x00, power off" | 001, 005a, 005b, 006 |
| 33 | Individual Real-Time Alerts (Device Fault, PIN Lockout) | **FaultComponent:** IR1, IR2, MQ2, KEYPAD, SPEAKER, POWER_MONITOR, DATABASE; remover PWM_DRIVER. **Trigger:** "first failure of a hardware component this boot, or database recreated at boot". PIN Lockout sem alterações | 004 |
| 34–35 | Individual Log Events | "No PUBACK: Not buffered; recovered by get_events" → "Event stays PENDING; resent after reconnection". "PUBACK: Done" → "Mark event SENT". Todos os eventos usam a outbox | 006 |
| 36 | Message Specification – Broker Connection | 0x86 e 0x87: "Retry every 5 min; log once" → "Retry with backoff (1 s → 60 s); log once per episode". Na nota, remover "auth errors every 5 min" | 006 |
| 37 | MQTT – Connection and Alerts Specification | Outbox para todos os eventos (INSERT PENDING → publish → SENT). Ordem na reconexão: online → status → eventos pendentes. Remover referências a `get_events`, se existirem (confirmar na execução) | 001, 006 |
| 38 | MQTT – Remote Command Specification | O evento de registo que se segue à resposta passa pela outbox. O resto mantém-se | 006 |
| 39 | MQTT – Connection Loss and Recovery | Remover "Wi-Fi loss ≤ 2 s (wpa_supplicant event)"; mantém-se "broker loss ≤ 60 s". Loop de reconexão: remover "(auth errors: every 5 min)". "Resend PENDING alerts" → "Resend PENDING events". Passo 18: "duplicates dropped by event_id" → "exact duplicates dropped; list in arrival order". Remover o passo 20 ("one 'N alerts received' notification") e todo o bloco "App sync" (passos 21–26) | 001, 006 |
| 40 | MQTT – Low Battery Shutdown Specification | **Redesenhar como "Low Battery and Safe State".** Bateria fraca a ≤ 6.7 V (`details` = tensão). A ≤ 6.4 V: log SHUTDOWN (PENDING), publicar se houver ligação, fechar a base de dados + `sync`, continuar a funcionar. O HAT corta a energia → o broker publica o Last Will passados ~45 s. A rede elétrica volta → o Pi arranca, repõe o último estado e envia os eventos pendentes. Remover os passos 14–21 (offline com motivo, DISCONNECT 0x00, power off, POWER_RESTORED no arranque seguinte) | 001, 004, 005a, 005b |
| 41 | Communication Stack | Caixa Application: "C++ NetworkMQTTThread" → "C++ CommThread (MqttLink)". (O "udhcpc" na nota de rodapé não vem destas decisões; ver CON-12 na Fase 4) | 003b |
| 42 | Wi-Fi Specification | "Link loss: wpa_supplicant event detected in ≤ 2 s" → "Link loss: detected by the MQTT keep-alive (≤ 60 s)" | 001 |
| 43 | Package diagram | **Redesenhar** com os packages da DEC-003b (`threads`, `logic`, `devices`, `hw`, `outputs`, `storage`, `net`, `core`), mais Cloud e Mobile App. Interfaces do sistema: `/dev/vigil_pwm` (driver próprio), libgpiod, i2c-dev e `/dev/watchdog` (usado pelo `main`) | 003b |
| 44–46 | Class Diagram (3 slides) | **Substituir** por: (a) no corpo, um class diagram simplificado por package (~12 classes, com interfaces, realizações real/simulado, `WorkerThread`, composição do `AlarmLogic` e direção das dependências); (b) no anexo, o diagrama completo por package. O diagrama simplificado tem de mostrar todos os nomes que o mapa SOLID (slide 70) refere, e leva uma nota SOLID (secção 5) | 003b, 009 |
| 47 | Task overview | **Redesenhar:** 5 threads + `main`. EventQueue (todas → controller) e CommQueue (controller + Paho → Comm). Heartbeats → `main` → `/dev/watchdog`. Remover as filas `OutputCmd`/`StorageCmd`/`NetCmd`, os fluxos "committed event", a caixa do Supervisor e os "4 timerfds" | 003a |
| 48 | Task priority | FIFO 80: `SafetySensorsThread`, `AlarmControllerThread`. FIFO 50: `LocalInputThread`, `PowerMonitorThread`. SCHED_OTHER: `CommThread`, `main`. Remover a banda FIFO 70. Acrescentar legenda ("higher priority ↑") e uma linha de justificação por banda | 003a |
| 49 | Intrusion FSM | Transição inicial: "[last state ARMED] → ARMED / [else] → DISARMED (no event logged)". O resto mantém-se | 004 |
| 50 | Fire FSM | A guarda "[smoke present]" no fim do silence period passa a usar o estado mantido pelos eventos `SMOKE_DETECTED`/`SMOKE_CLEARED` (sem leitura de outra thread). O resto mantém-se | 003a |
| 51 | Power management FSM | **Arranque:** estado inicial silencioso, sem alertas POWER_RESTORED (BOOT) nem POWER_LOST. **LOW_BATTERY:** guarda ≤ 6.7 V. **SHUTDOWN → SAFE_STATE:** ação de entrada "SHUTDOWN alert, close DB, sync; keeps running". Remover o self-loop do nível de bateria. POWER_LOST sem percentagem | 001, 005a, 005b |
| 52 | Connectivity LED FSM | Notas: remover "Wi-Fi loss ≤ 2 s (netlink)"; "NetworkMQTTThread raises EVT_NET_RESTORED" → "CommThread (MqttLink)"; reconexão com uma só regra (1 s → 60 s). Se continua a ter slide próprio (agora é um campo do `AlarmLogic`) decide-se na Fase 4 | 001, 003b, 006 |
| 53 | Flowchart – StartUp | **Redesenhar:** abrir cada linha GPIO em separado (falha → FAULT e arranque degradado); abrir `/dev/vigil_pwm` (falha → FAULT(SPEAKER)); base de dados: abrir + `quick_check` → OK: carregar o PIN / falha: renomear o ficheiro, base nova, hash do PIN por omissão, FAULT(DATABASE) / primeiro arranque: criar, sem FAULT. Sem ramo de backup. Ler o último ARMED/DISARMED → estado inicial. Criar EventQueue + CommQueue. Abrir o watchdog. Arrancar as 5 threads. O estado de energia é decidido em silêncio pelas 3 primeiras amostras. Aquecimento do MQ-2 em paralelo. O `main` entra no ciclo de supervisão | 001, 003a, 004, 005b |
| 54 | Flowchart – Control Loop | **Redesenhar:** esperar na EventQueue até ao próximo prazo (máx. 1 s) → esvaziar a fila → `AlarmLogic.handle(event, now)` / prazos expirados → `AlarmOutputs.apply()` → `CommCommand`s → heartbeat. Sem epoll, eventfd nem timerfd. **Fundir com o slide 57** (descrevem a mesma thread) | 003a, 003b |
| 55 | Flowchart – PIN validation | Atualizar os nomes (`AlarmLogic` / `PinGuard`). A lógica mantém-se | 003b |
| 56 + 59 | Flowchart – MQ2SensorThread + IRSensorsThread | **Substituir por um só fluxograma da `SafetySensorsThread`:** a cada 100 ms, ler IR1, IR2 e MQ-2 → `DebouncedInput` (2 leituras para o IR; 5 leituras + aquecimento para o MQ-2) → transições → eventos. Falha de um sensor → FAULT uma vez, os outros continuam. Heartbeat | 003a, 003b, 004 |
| 57 | Flowchart – AlarmControllerThread | Fundir com o slide 54 | 003a |
| 58 | Flowchart – OutputThread | **Remover** (a thread deixou de existir; as saídas são aplicadas no ciclo do controller) | 003a |
| 60 | Flowchart – LocalInputThread | Atualizar: `KeypadScanner` + `PinEntry` + `DebouncedInput` (botão). Falha das linhas → FAULT(KEYPAD). O heartbeat é verificado pelo `main` (já não pela SupervisorThread) | 003b, 004 |
| 61 | Flowchart – PowerMonitorThread | Atualizar: `PowerSupplyMonitor`; regras de 3 amostras (perda/regresso da rede elétrica, ≤ 6.7 V uma vez por falha, ≤ 6.4 V crítico); sem eventos de nível de bateria; falhas I²C consecutivas → FAULT(POWER_MONITOR); estado inicial silencioso | 001, 004, 005a, 005b |
| 62 + 63 | Flowchart – StorageThread + NetworkMQTTThread | **Substituir por um só fluxograma da `CommThread`:** ler a CommQueue (com timeout) → LogEvent → `EventStore.append` (PENDING) → `OutboxPublisher` publica os pendentes por ordem do `id`, ≤ 5/s → SENT no PUBACK. `MqttLink`: ligação e backoff de 1 a 60 s; no CONNACK: online → status → pendentes. `cmd` recebido → `Protocol.parse` → EventQueue. SavePin → `PinStore`. EnterSafeState → fechar a base de dados. Falha de escrita → syslog. SIGTERM → DISCONNECT 0x04 | 003a, 003b, 004, 005b, 006 |
| 64 | Flowchart – SupervisorThread | **Substituir pelo fluxograma do ciclo do `main`:** a cada 1 s, o `HealthMonitor` verifica os heartbeats (timeouts 1 / 1 / 3 / 3 / 10 s) → todos frescos? alimentar o watchdog : deixar de alimentar (reset em 15 s). SIGTERM → parar as threads, DISCONNECT 0x04, magic close do watchdog | 001, 003a, 004, 005b |
| 65 | Linux Device Driver | **UAPI:** substituir a tabela `pattern_id` por um comando de sequência (canal, repetir, número de passos ≤ N, passos {frequency_hz, duty, duration_ms}). "Tone and blink patterns stepped in the kernel" → "Sequences supplied by user space, stepped by hrtimers". Remover "Repeating patterns become the channel's base; one-shots play over it…". "OutputThread is the only client" → "AlarmOutputs (in AlarmControllerThread) is the only client". Mantêm-se: duty ≤ 50 %, saída LOW quando parada, `release()` fail-loud e os códigos de erro (EINVAL passa a incluir sequências inválidas) | 002, 003a |
| 66 | Linux Device Driver – Architecture | **Redesenhar:** user space `AlarmControllerThread` → `AlarmOutputs` → `PwmDevice.write(sequence)`; kernel: char device (open/write/release, mutex) → sequence player por canal (hrtimer) → API de consumidor PWM → `pwm-bcm2835` → GPIO18 / GPIO13. Remover as tabelas de padrões e a lógica base/one-shot | 002 |
| 67 | Linux Device Driver – Flowchart | **Redesenhar:** `write()` (tamanho/cópia/validação da sequência → lock → cancelar o timer → aplicar o passo 0 → arrancar o timer se houver mais passos) e callback do hrtimer (passo seguinte; no fim: repetir desde o início ou parar com a saída LOW) | 002 |
| 68 | Design Patterns | **Reescrever** com os 8 padrões da secção 5: saem Command, Active Object, Observer (`IEventSink`) e Supervisor; entra Composition Root; atualizar as classes de Producer–Consumer, Adapter, Transactional Outbox e Watchdog | 003a/b, 009 |
| 69 | Threads Responsibilities | Refazer com as 5 threads + `main` (tabela da DEC-003a) e os timeouts de heartbeat da DEC-004 (4.5) | 003a, 004 |
| 70 | SOLID Principles | **Substituir** pelo mapa de evidência da secção 5 (Principle · Where · Evidence). Saem "Each thread has one job", "new event = EventType value", "New tone = row in the driver pattern table", "The 8 WorkerThread subclasses used by SupervisorThread" e `IEventSink` | 003b, 009 |
| 71 | Parameters Times | Remover "Pi WiFi loss detection ≤ 2 s" e "Reconnect after auth error every 5 min". "Reconnect backoff 1 s → 60 s (all failures)". Acrescentar: bateria fraca 6.7 V; crítico 6.4 V (safe state); timeouts de heartbeat; watchdog 15 s; períodos de amostragem 15 ms / 100 ms / 1 s; confirmação do IR em 2 leituras; MQ-2 com 5 leituras e 60 s de aquecimento | 001, 004, 005a, 006 |
| 72 | GUI Interface (tabela de estados) | Verificar que não aparece percentagem de bateria nem histórico. O resto mantém-se | 001, 006 |
| 73–75 | GUI Interface (mock-ups) | Remover a percentagem de bateria, se aparecer. A lista mostra os últimos 20 eventos (alertas e registo) por ordem de chegada. Sem resumo de reconexão nem histórico. Texto da notificação de intrusão: "Intrusion – door/window moved (IR1)" | 001, 006, 010 |
| 76–85 | Test cases (10 slides) | **Substituir por 5 slides:** 1 de estratégia + 4 de testes de aceitação (secção 5, "Testes"). Os testes atuais passam para o catálogo completo no anexo ou no relatório, corrigidos pelas decisões: remover "Event Sync", "History Query" e "Driver Concurrency"; "Start-up: Power on → DISARMED" → "restores the last ARMED/DISARMED state"; "Retry every 5 minutes" → "retry with backoff up to 60 s"; "safe shutdown" → "safe state"; "Wi-Fi Loss" detetada em ≤ 60 s; "Write channel, frequency, duty, enable" → "Write step sequences"; acrescentar o glitch no IR, os FAULT por componente e a base de dados corrompida no arranque. Não entram a coluna "Method" nem referências a equipamento de laboratório | 001–006, 008 |
| 86 | Error Handling – Critical Cases | **Refazer** a partir da tabela da DEC-004 (4.3), com as colunas Component / Response / What keeps working. Remover "database rebuilt", "IR or MQ-2 stuck" e "Supervisor restarts it". Acrescentar "Unexpected error → watchdog reset → last state restored" | 004 |
| 87 | Gantt Chart | Sem alterações por decisões (Fase 4) | — |
| 88 | Thank you | — | — |

## 5. Conteúdos novos exigidos pelas decisões

- **Vista de concorrência:** 5 threads + 2 filas. Pode ser o slide 47 refeito (DEC-003a).
- **Class diagram simplificado por package** no corpo da apresentação e diagrama completo no anexo (DEC-003b).
- **Novos fluxogramas:** `SafetySensorsThread`, `CommThread` e o ciclo do `main` (DEC-003a).
- **Justificação das diferenças face à Análise**, uma linha em cada slide afetado:
  - o sistema repõe o último estado armado/desarmado no arranque, em vez de "starts disarmed" (deck A, slide 15) — DEC-004;
  - o ERD passa de 3 para 2 tabelas — DEC-006;
  - a Análise assumia um sensor PIR (deck A, slide 15); o Design deteta o movimento da porta/janela com um IR refletivo no aro — DEC-010. Texto: "The Analysis assumed a PIR sensor. The design detects movement at the entry points: an IR reflective sensor on each door/window frame."

### Testes (DEC-008)

**Slide "Test Strategy"** (texto para o slide):

- **L1 – Host unit tests (GoogleTest):** alarm logic with injected time, devices on simulated hardware, storage on in-memory SQLite, message protocol.
- **L2 – Host integration:** simulation build + Mosquitto broker + mobile app.
- **L3 – Raspberry Pi:** real hardware, custom driver, power, real-time under load, EMQX + mobile app.
- Every Analysis requirement is covered by at least one acceptance test.
- The same contract tests run on simulated and real hardware.
- Full test catalogue in the annex.

**Slides "Acceptance Tests"** (4 slides, 6 testes por slide):

| ID | Objective | Stimulus | Expected result | Level | Req. |
|---|---|---|---|---|---|
| AT-01 | Local arming with exit delay | Disarmed; press the arm button | Short beep; red LED on; ARMED when the exit delay ends; event shown in the app | L3 | Flexible Control, Entry/Exit Delay, Visual Status |
| AT-02 | Disarm during the entry delay | Armed; open the door (IR1); correct PIN before the delay ends | Warning tone during the delay; DISARMED; no alarm, no alert | L3 | Entry/Exit Delay, Flexible Control |
| AT-03 | Intrusion alarm | Armed; open the door; let the entry delay expire | Intrusion tone; red LED flashing; INTRUSION alert on the phone ≤ 5 s; on until disarmed | L3 | Dual Detection, Distinct Alarms, Instant Alerts |
| AT-04 | PIN lockout | Enter 5 wrong PINs | Error beep each time; PIN_LOCKOUT alert; PIN entry ignored until the lockout ends | L1, L3 | Security, Flexible Control |
| AT-05 | Fire alarm in any state | Smoke on the MQ-2 when disarmed, in exit delay and armed | Fire tone, distinct from intrusion; FIRE alert in all three states | L3 | Dual Detection, Distinct Alarms |
| AT-06 | Fire silence period | Fire alarm; correct PIN; smoke kept until the period ends | Silenced; smoke ignored during the period; alarm and alert again if smoke remains | L1, L3 | Fire Alarm Silence Period |
| AT-07 | Remote control | App: arm; disarm with PIN; disarm during a fire alarm | Armed at once (no exit delay); disarmed; disarm during fire rejected, alarm continues | L2, L3 | Flexible Control |
| AT-08 | PIN change from the app | Wrong current PIN; then correct PIN; reboot | Wrong: error, PIN unchanged. Correct: new PIN works after reboot; only a salted hash stored | L2 | PIN Management, Security |
| AT-09 | IR glitch rejection | Armed; one isolated 100 ms pulse on the IR input | No event; no entry delay | L1 | Dual Detection |
| AT-10 | MQ-2 warm-up | Cold boot in clean air | No smoke event during or after warm-up | L3 | Dual Detection |
| AT-11 | Mains failure and return | Unplug the 8.4 V adapter; plug it back | No reset; monitoring continues; POWER_LOST / POWER_RESTORED alerts; RGB colour changes | L3 | Power Resilience, Instant Alerts, Visual Status, Availability |
| AT-12 | Low battery and safe state | On battery to ≤ 6.7 V, then ≤ 6.4 V; reconnect mains | LOW_BATTERY with voltage; SHUTDOWN alert, database closed, system keeps running; Pi boots on mains with the last state restored | L3 | Power Resilience |
| AT-13 | Battery autonomy | Typical load on battery until safe state | ≥ 3 h (theoretical ≈ 4 h) | L3 | Power Resilience |
| AT-14 | Offline events | Stop the broker; generate events; restart it | RGB shows loss; local alarm works; all events delivered in order, ≤ 5 per second; no duplicates in the app | L2, L3 | Pending Notification Handling, Connectivity Monitoring, Event Logging |
| AT-15 | Offline detection | Cut the Pi's power; separately, stop the service | Power cut: "offline" within 1.5 × keep-alive. Planned stop: "offline" at once | L2, L3 | Instant Alerts |
| AT-16 | Secure channel | Wrong CA; app publishes on a device topic; expired command | Connection refused; publish rejected by the broker; command not executed | L2, L3 | Security |
| AT-17 | State after reboot | Reboot while armed; reboot while disarmed | Returns ARMED / DISARMED; a door already open does not trigger | L2, L3 | Availability |
| AT-18 | Watchdog recovery | Block one thread (test build) | Pi reset by the watchdog; last state restored | L3 | Availability |
| AT-19 | Component fault | Simulated read failure on IR1 | FAULT(IR1) sent once; everything else keeps working | L2 | Availability |
| AT-20 | Corrupt database at boot | Corrupt the database file; boot | New database, default PIN, FAULT(DATABASE), DISARMED | L2, L3 | Event Logging, Availability |
| AT-21 | Custom PWM driver | Write a valid sequence; an invalid one; close the device | Output follows the steps; invalid → EINVAL; output LOW when stopped | L3 | Distinct Alarms, Visual Status |
| AT-22 | Alarm latency under load | CPU stress, database writes and a pending backlog; then smoke or door opening | Alarm output ≤ 1 s | L3 | Real-Time Responsiveness |
| AT-23 | Keypad response and periodic tasks | Key presses and sampling loops under the same load | Key → beep ≤ 100 ms; 15 ms and 100 ms loops with no missed period | L3 | Real-Time Responsiveness |
| AT-24 | Long-term stability | 24 h with periodic events | No crash, no reset, no memory growth | L3 | Availability |

### SOLID e padrões (DEC-009)

**Slide 70 — "SOLID Principles"** (texto para o slide):

| Principle | Where | Evidence |
|---|---|---|
| S – Single responsibility | Threads only schedule; `AlarmLogic` decides; `OutputPolicy` owns the output rules; `EventStore` / `OutboxPublisher` / `MqttLink` / `Protocol` | A new tone changes only `OutputPolicy`; a new message format changes only `Protocol` |
| O – Open/closed | `IGpioInput`, `IPwm`, `IPowerSensor`, `IMessageTransport`; `DebouncedInput` configured by parameters; tones as step sequences | New sensor = new `DebouncedInput` instance; new tone = new sequence, driver unchanged; simulated hardware = new class, callers unchanged |
| L – Liskov substitution | `WorkerThread` subclasses; real and simulated hardware classes | Same contract tests pass on simulated (L1/L2) and real (L3) hardware; one heartbeat and error policy for all threads |
| I – Interface segregation | `IGpioInput` / `IGpioOutput` / `IPwm` / `IPowerSensor`; `EventStore` vs `PinStore`; `CommCommand` as small structs | No client depends on methods it does not use |
| D – Dependency inversion | `logic` depends on nothing; devices and outputs depend on hardware interfaces; `OutboxPublisher` on `IMessageTransport`; `main` is the composition root | `AlarmLogic` tested on the laptop with no hardware and injected time |

**Slide 68 — "Design Patterns"** (texto para o slide):

| Pattern | Where (classes) | Problem it solves |
|---|---|---|
| Producer–Consumer | LocalInput, SafetySensors, PowerMonitor → EventQueue → AlarmControllerThread; controller + Paho callbacks → CommQueue → CommThread | Many asynchronous sources, one deterministic decision maker; the controller never blocks on disk or network |
| Monitor Object | `BlockingQueue<T>` (priority-inheritance mutex + monotonic condition variable) | Thread-safe hand-over without priority inversion; wait until the next deadline |
| Template Method | `WorkerThread`: wait → step() → heartbeat; one error policy | Same life cycle, heartbeat and error handling for the 5 threads |
| Pure FSM logic | `AlarmLogic` + `IntrusionFsm`, `FireFsm`, `PowerFsm`, `PinGuard`: no threads, no I/O, time injected | Behaviour unit-tested on a laptop |
| Adapter | `GpioLines` (libgpiod v1), `Ina219` (i2c-dev), `PwmDevice` (/dev/vigil_pwm), `MqttLink` (Paho), `Database` (SQLite) | Hides C APIs; a libgpiod v1 → v2 change touches one class |
| Composition Root | `main()` builds real or simulated hardware objects and injects them | The same code runs on the Pi and on the laptop |
| Transactional Outbox | `EVENT_LOG.status` / `sent_at` + `OutboxPublisher` | No event lost between the database and MQTT delivery |
| Watchdog | `main` feeds /dev/watchdog only while every heartbeat is fresh | A hung thread or unexpected error resets the Pi; last state restored |

**Notas SOLID ao longo da apresentação** (uma linha por slide, em rodapé; números do deck atual):

| Slide | Nota |
|---|---|
| 43–46 Package / class diagram | "SOLID – D: the alarm logic depends on nothing; main() injects real or simulated hardware." |
| 16 IR interface | "SOLID – O: IR1, IR2, MQ-2 and the arm button are instances of one DebouncedInput class, configured by parameters." |
| 47 / 69 Threads | "SOLID – S: threads only schedule; each component they call has one responsibility." |
| 26 Data storage (ou o fluxograma da CommThread) | "SOLID – S/I: EventStore stores, OutboxPublisher sends, Protocol formats; PinStore is a separate interface for PIN data." |
| 65 Linux device driver | "SOLID – O: a new tone is data (a step sequence); the driver and its UAPI do not change." |
| Test Strategy (DEC-008) | "SOLID – L: the same contract tests run on simulated and real hardware." |

## 6. Slides que dependem de decisões pendentes

| Pendente | Slides |
|---|---|
| Fase 4 (estrutura) | Agenda (2), ordem e narrativa, anexo, Gantt (87), slide 52 como slide próprio ou não, e problemas que não vêm destas decisões (CON-12 "udhcpc", texto do slide 6 reutilizado do G4, números de slide, gralhas nos títulos 54, 59 e 71) |

## 7. Efeito no tamanho do deck

- **Só por estas decisões, o deck perde pelo menos 9 slides:** remove-se o slide 58, fundem-se três pares (54 + 57, 56 + 59 e 62 + 63) e os 10 slides de testes passam a 5 (DEC-008). Os slides 44–46 podem passar de 3 para 1 no corpo, com o resto no anexo.
- **A redução maior virá da Fase 4:** mover o detalhe (tabelas de interface, especificação completa de mensagens, diagrama de classes completo) para o anexo e organizar a apresentação por cenários.
