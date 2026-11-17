> **ARCHIVED — superseded on 2026-11-09 by `VIGILSYSTEM_MASTER_CONTEXT.md` v1.0. Kept for history only. Do not implement from this document.**
>
> Original location: Claude Project: `claude/VigilSystem – Simplification Decision Log.md` · Document date: 8–9 Nov 2026 · Superseded by: Master Context §G (DEC-001…DEC-012), §H.
> The text below this box is an unchanged copy of the original.

---

# VigilSystem – Registo de Decisões de Simplificação

Grupo 8 · David Cardoso (PG63873) · Ana Sofia Cunha (A107017) · Criado a 8 nov 2026 · Atualizado a 9 nov 2026 (DEC-010)

## 1. Objetivo e regras de uso

Este documento regista as decisões da revisão de simplificação do VigilSystem, iniciada a 8 nov 2026. Cada decisão aprovada indica: problema, solução anterior, alternativas consideradas, recomendação, riscos e consequências, decisão tomada e estado.

- **Aprovar não é executar.** As alterações a documentos, slides, diagramas e código só são feitas com autorização explícita, normalmente em bloco.
- **Atualização deste registo.** Por autorização permanente do utilizador (8 nov), este ficheiro, e só este, é atualizado assim que uma decisão fica completa.
- **Documentos desatualizados.** Até essa execução, o Implementation Guide (4 nov), o Decisions Record (2 nov), o deck de Design (88 slides) e as instruções do Project estão desatualizados nos pontos aqui decididos. A regra de precedência entre documentos (CON-08) ainda não foi decidida.
- **Decisões rejeitadas** nunca são tratadas como aprovadas. **Decisões aprovadas** só se reabrem perante um problema novo e com autorização.
- **SOLID Impact Analysis.** Todas as propostas de simplificação arquitetural incluem esta análise. O SOLID é requisito do professor e deve ser aplicado de forma pragmática: cada abstração tem de ter um benefício concreto.
- **Alterações à apresentação.** As listas de slides em "Alterações pendentes de execução" são indicativas. A lista consolidada e verificada slide a slide está em `claude/VigilSystem – Presentation Changes (DEC-001 to DEC-006).md`. O nome do ficheiro mantém-se até haver autorização para o mudar, mas o conteúdo já cobre até à DEC-010.

## 2. Contexto confirmado

- Todos os requisitos da Análise (deck A, slides 5–7) se mantêm.
- O SOLID é requisito explícito do professor.
- O professor não valoriza nenhum tipo de driver em particular.
- **Compras (9 nov).** Disponíveis: Raspberry Pi, LED vermelho, LED RGB, altifalante, os 2 sensores IR, a breadboard e o cartão microSD. Tudo o resto falta comprar, incluindo o UPS HAT, as baterias 18650, o teclado, os botões e o MQ-2; a encomenda está prevista para a semana de 9 nov. A lista consolidada está na DEC-007.

## 3. Tabela de decisões

| ID | Decisão | Depende de | Estado |
|---|---|---|---|
| DEC-001 | Âmbito do entregável | — | **Aprovada: B + M1** (item do encerramento alterado pela DEC-005b) |
| DEC-002 | Estratégia do device driver | 001 | **Aprovada: B** |
| DEC-003a | Modelo de concorrência | 001, 002 | **Aprovada: B' + espera por prazos** |
| DEC-003b | Responsabilidades e dependências entre classes (SOLID) | 003a | **Aprovada: B** |
| DEC-004 | Supervisão e tratamento de erros | 001, 003 | **Aprovada: 4.1 b, 4.2 b, 4.3 FAULT só para componentes + BD no arranque, 4.4 a, 4.5 valores por omissão** |
| DEC-005a | Limiar de bateria fraca em tensão | 001 | **Aprovada: 6,7 V** |
| DEC-005b | Restante gestão de energia | 001, 005a | **Aprovada: 5b.1 c, 5b.2 confirmado, 5b.3 b, 5b.4 a** |
| DEC-006 | Protocolo remoto e âmbito da app | 001, 003 | **Aprovada: 6.1 b', 6.2 a, 6.3 b, 6.4 a** |
| DEC-007 | Hardware em aberto | — | **Aprovada: 7.1 b, 7.2 b, 7.3 b** |
| DEC-008 | Estratégia e conjunto de testes de aceitação | todas | **Aprovada: 8.1 b, 8.2 B, 8.3 b (método só como nota, fora da apresentação)** |
| DEC-009 | Mapa de evidência SOLID | 003b | **Aprovada: 9.1 B + notas SOLID ao longo da apresentação, 9.2 b, 9.3 não apresentar** |
| DEC-010 | Sensor de intrusão face ao requisito "movement" da Análise | 007 | **Aprovada: A' — concretizar o requisito (movimento da porta/janela), só com os 2 IR** |
| DEC-011 | Precedência e destino dos documentos (CON-08, CON-01) | todas | Pendente |

## 4. Decisões aprovadas

### DEC-001 — Âmbito do entregável

- **Problema.** Durante o Design foram acrescentadas cerca de 12 funcionalidades além dos requisitos da Análise, todas com a mesma prioridade. Sem um âmbito fechado, a especificação continuava a crescer: o deck passou de 61 para 88 slides entre 2 e 8 nov.
- **Solução anterior.** O âmbito era tudo o que está no Implementation Guide.
- **Alternativas.**
  - A — Mínimo: só os requisitos da Análise.
  - B — Equilibrado.
  - C — Completo: manter tudo.
- **Recomendação.** B. Mantém todos os compromissos da Análise e os acréscimos baratos e defensáveis.
- **Decisão.** **B + M1**: as funcionalidades adiadas não aparecem na apresentação de Design.

| Categoria | Itens | Na apresentação |
|---|---|---|
| Manter | Todos os requisitos da Análise; PIN lockout; 2.º sensor IR | Sim |
| Simplificar | Watchdog alimentado só com heartbeats frescos, sem reinício de threads; LOW_BATTERY e, abaixo do limiar crítico, **estado seguro sem desligar** (alterado pela DEC-005b; originalmente registo + `sync` + `poweroff`); arranque degradado + um alerta FAULT, sem pisca RGB nem novas tentativas; falha da base de dados → base nova com PIN por omissão + FAULT | Sim, na forma simplificada |
| Adiar | % de bateria em passos de 10 %; alertas de energia no arranque (POWER_RESTORED/BOOT, POWER_LOST ao arrancar a bateria); backup e restauro da base de dados; histórico `get_events`; resumo de reconexão na app | **Não (M1)** |
| Remover | Deteção de perda de Wi-Fi em ≤ 2 s por netlink (`LinkMonitor`) | Não |

- **Alteração posterior.** O item "abaixo do limiar crítico: `poweroff`" foi reaberto e substituído na DEC-005b (5b.3 b), por autorização do utilizador. O problema novo era a issue 7 do Work Log: um Pi desligado pode não voltar a arrancar quando a rede elétrica regressa.
- **SOLID.** Efeito neutro a positivo:
  - L melhora: sem reinício de threads, todas as subclasses cumprem o contrato do `WorkerThread`.
  - D melhora: sem `get_events`, desaparece a dependência concreta Network→Storage.
  - S mantém-se: o lockout fica numa classe pequena.
- **Riscos e consequências.**
  - A perda de Wi-Fi passa a ser detetada pelo keep-alive do broker em ≤ 60 s, em vez de ≤ 2 s.
  - Deixa de haver histórico remoto.
  - A base de dados deixa de ter backup.
  - Os eventos de registo gerados com o Pi offline deixavam de chegar à app; resolvido na DEC-006 (6.1 b').
- **Alterações pendentes de execução** (lista indicativa, a confirmar na execução):
  - **Implementation Guide:** §5.4, §6.5 (P-1, P-2, P-9), §6.9, §7.1, §7.9, §9.5, §10.4, §10.8, §14, §16; §17 D-14, D-18, D-34, D-35, D-36.
  - **Decisions Record:** as secções correspondentes.
  - **Slides dos itens adiados:**
    - % de bateria: 23, 30, 47, 61, 69 e a GUI, se a mostrar;
    - alertas de energia no arranque: 32, 40, 51, 53;
    - backup da base de dados: 53, 62, 84, 86;
    - `get_events`: 29, 34–35, 37, 39, 45, 47, 63, 83–84.
  - **Slides do item removido (Wi-Fi netlink):** 39, 42, 52, 71, 83, 85.
  - **Slides dos itens simplificados:**
    - watchdog: 64, 68, 69, 86;
    - encerramento: 40, 51;
    - FAULT: 20, 33, 86;
    - falha da base de dados: 53, 84, 86.
  - **Outros documentos:** o documento da GUI e o class diagram.

### DEC-002 — Estratégia do device driver

- **Problema.**
  - A política de saídas estava em três sítios: o kernel, o `AlarmLogic` e a OutputThread.
  - Os padrões eram fixos no kernel, por isso mudar um tom obrigava a recompilar o módulo.
  - O driver está no caminho crítico de todo o som e de todo o LED vermelho, e ainda não foi começado.
- **Solução anterior.** `vigil_pwm.ko` com 8 padrões fixos no kernel, regra base + one-shot e passos por hrtimers.
- **Alternativas.**
  - A — Manter.
  - B — "Sequence player" genérico.
  - C — Driver mínimo (frequência, duty, enable).
  - D — Driver de entradas com interrupções, com som e LED a passar para o sysfs.
- **Recomendação.** B.
- **Decisão.** **B.**
- **O que fica igual.** Platform driver sobre o `pwm-bcm2835`, overlay próprio, canal 0 (altifalante, GPIO18) e canal 1 (LED vermelho, GPIO13), hrtimers, mutex, validação que protege o hardware (duty do altifalante ≤ 50 %, saída LOW quando parada) e `release()` fail-loud.
- **O que muda.**
  - O `write()` passa a receber uma sequência de até N passos {frequência, duty, duração} e um indicador "repetir".
  - O kernel deixa de ter padrões e a regra base/one-shot.
  - Os tons e a política de saídas ficam só em user space (`OutputPolicy`).
  - O user space compõe o "beep por cima do aviso": envia o beep e, quando acaba, repõe o aviso, usando um prazo no controller.
- **SOLID.**
  - S melhora: a `OutputPolicy` passa a ser a única dona da política de saídas.
  - O melhora: um tom novo é só dados; não muda nem o driver nem a UAPI.
  - L melhora: a implementação simulada de `IPwm` só tem de "tocar uma sequência".
  - I e D mantêm-se: `IPwm` fica pequena e a UAPI fica confinada ao `PwmDevice`.
- **Riscos e consequências.**
  - O user space tem de repor a base depois de um beep.
  - O primeiro passo técnico, a fazer cedo, é ter o overlay, o `probe()` e um tom constante a funcionar no Pi.
  - O controller deixa de precisar da OutputThread, porque o `write()` devolve de imediato.
- **Alterações pendentes de execução:**
  - slide 65 (tabela UAPI: de `pattern_id` para sequência de passos);
  - slides 66–67 (arquitetura e fluxogramas do driver);
  - Guide §8.3 (UAPI) e §8.4 (as tabelas de tons passam para a `OutputPolicy`);
  - testes do driver no slide 80.

### DEC-003a — Modelo de concorrência

- **Problema.** Havia 9 threads, 4 filas, epoll + eventfd + 4 timerfds, o `IEventSink` e leituras cruzadas à base de dados. Depois das DEC-001 e DEC-002, várias threads perderam a razão de existir.
- **Alternativas.**
  - A — 3 threads.
  - B — 6 threads.
  - B' — 5 threads.
  - C — 9 threads (manter).
- **Recomendação.** B'. A recomendação mudou de B para B' depois da comparação detalhada, por três razões:
  1. em B, o primeiro glitch no IR dispara logo o evento, e B' filtra-o com a confirmação em 2 leituras;
  2. B usa a API de eventos da libgpiod, que é onde a v1 (Pi) e a v2 (portátil) mais diferem;
  3. em B', a simulação só tem de devolver níveis, o que a torna mais fiel.
- **Decisão.** **B'**, com o mecanismo de espera por prazos.

| Thread | Ativação | Prioridade | Papel |
|---|---|---|---|
| LocalInput | Periódica, 15 ms | SCHED_FIFO 50 | Teclado + botão de armar |
| SafetySensors | Periódica, 100 ms | SCHED_FIFO 80 | IR1, IR2 (confirmação em 2 leituras, um evento por abertura) e MQ-2 (5 leituras; emite "detetado" e "limpo") |
| PowerMonitor | Periódica, 1 s | SCHED_FIFO 50 | INA219 |
| AlarmController | Espera na EventQueue até ao próximo prazo (máx. 1 s) | SCHED_FIFO 80 | Decide; aplica as saídas diretamente (driver + RGB) |
| Comm | CommQueue | SCHED_OTHER | SQLite + MQTT (outbox) |
| `main` | Ciclo de 1 s depois do arranque | SCHED_OTHER | Heartbeats → watchdog; SIGTERM |

- **Filas.**
  - EventQueue: de todas as threads para o controller.
  - CommQueue: do controller e dos callbacks do Paho para a Comm.
- **Mecanismo de espera (aprovado).**
  - Os prazos (exit delay, entry delay, silence period, lockout e reposição do aviso depois de um beep) passam a ser estado da lógica, com o tempo injetado.
  - O controller espera na EventQueue até ao prazo mais próximo, no máximo 1 s, para enviar o heartbeat.
  - Primitivas: `pthread_cond` com `CLOCK_MONOTONIC` (`pthread_condattr_setclock`) e `pthread_mutex` com `PTHREAD_PRIO_INHERIT`, encapsuladas na classe da fila.
  - Substitui o epoll + eventfd + 4 timerfds. Mantém o comportamento e a imunidade a acertos do NTP, com uma só primitiva de espera e pthreads diretamente.
- **Eliminado:**
  - OutputThread;
  - a separação Storage/Network;
  - o `IEventSink`;
  - a SupervisorThread, cujo papel passa para o `main`;
  - as threads IRSensors e MQ2Sensor, que se juntam na SafetySensors;
  - a leitura de `isSmokePresent()` a partir de outra thread;
  - o epoll, o eventfd e os timerfds.
- **SOLID.** Depende da DEC-003b. As threads devem ser ciclos finos que chamam classes com uma responsabilidade cada, por exemplo:
  - classes de sensor;
  - `AlarmOutputs`;
  - `EventStore`, `MqttLink` e `OutboxPublisher`;
  - `HealthMonitor`.
  
  `IGpioInput` perde o `waitEdge`. Com os prazos na lógica e o tempo injetado, a lógica deixa de depender de timers do sistema operativo (D melhora). Regra: fundir threads, nunca responsabilidades.
- **Trade-offs aceites.**
  - Perde-se a task ativada por evento de hardware e o nível FIFO 70.
  - A deteção do IR pode demorar até ~200 ms, o que é irrelevante face ao entry delay de 30 s.
  - Um erro num sensor tem de ser tratado sem parar a leitura dos outros.
- **Consequências.** A contradição CON-02 ("any edge" vs "rising edge") desaparece.
- **Alterações pendentes de execução:**
  - slide 16 (interface do IR);
  - slides 47–48 e 69 (task overview, prioridades, tabela de threads, incluindo "4 timerfds" e "epoll");
  - slide 54 (control loop: sem epoll, eventfd nem timerfds; espera até ao próximo prazo);
  - os fluxogramas dos slides 56 e 59 juntam-se num só;
  - os slides 58, 62 e 63 são substituídos pelo fluxograma da Comm;
  - o slide 64 passa a ser o ciclo do `main`;
  - slides 43–46 (via DEC-003b) e 68 (via DEC-009);
  - testes 77 (acrescentar um teste de glitch no IR), 80 e 85;
  - Guide §5 (incl. §5.3 filas e §5.6 RT-4), §6.2 (BR-LOOP-1 a 3) e §7;
  - Decisions Record (arquitetura, threads, prioridades e comunicação entre threads).

### DEC-003b — Responsabilidades e dependências entre classes

- **Problema.** O modelo de classes seguia a regra "uma thread = uma classe = uma responsabilidade" (decisão D-03 e slide 70). Depois da DEC-003a, as threads deixaram de coincidir com responsabilidades. O modelo anterior tinha também problemas concretos:
  - a `AlarmControllerThread` guardava todo o estado e 6 handlers, enquanto o `AlarmLogic` só tinha `handle()`;
  - a `NetworkMQTTThread` acumulava cerca de seis responsabilidades;
  - a `StorageThread` construía o JSON dos alertas;
  - havia dependências concretas entre threads;
  - os comandos eram "structs-união" (`OutputCmd`, `StorageCmd`, `NetCmd`).
- **Solução anterior.** Cerca de 32 classes, interfaces e structs, mais 9 enums (slides 44–46).
- **Alternativas.**
  - A — As classes seguem as threads (~20 tipos).
  - B — Fundir threads, nunca responsabilidades (~35 tipos).
  - B− — Igual a B, com o `AlarmLogic` numa só classe (~30 tipos).
  - C — SOLID maximizado: interface para cada componente, State pattern, registo de handlers (~50 tipos).
- **Recomendação.** B.
- **Decisão.** **B.** B− fica como recurso declarado se o tempo apertar; ativá-lo exige uma nova decisão.

| Package | Classes | Responsabilidade |
|---|---|---|
| `threads` | `WorkerThread` (abstrata); `LocalInputThread`, `SafetySensorsThread`, `PowerMonitorThread`, `AlarmControllerThread`, `CommThread` | Só agendamento: esperar, chamar componentes, heartbeat |
| `logic` (sem E/S, tempo injetado) | `AlarmLogic` (coordena, dispara Fire primeiro, guarda os prazos e o estado de ligação); `IntrusionFsm`; `FireFsm`; `PowerFsm`; `PinGuard` (lockout); `PinHasher`; `OutputPolicy` (estado → saídas desejadas) | Regras |
| `devices` | `DebouncedInput` (4 instâncias: IR1, IR2, MQ-2, botão); `KeypadScanner`; `PinEntry` (#, *, timeout, 8 dígitos); `PowerSupplyMonitor` (histerese, 6,7 V / 6,4 V) | Transformar sinais em factos |
| `hw` | `IGpioInput`, `IGpioOutput`, `IPwm`, `IPowerSensor`; `GpioLines`, `PwmDevice`, `Ina219` (+ versões simuladas) | Acesso ao hardware |
| `outputs` | `AlarmOutputs` | Converter as saídas desejadas em sequências para o driver e em cor para o RGB |
| `storage` | `Database` (ligação RAII); `EventStore` (eventos + outbox); `PinStore` | SQLite |
| `net` | `IMessageTransport`; `MqttLink` (Paho, TLS, reconexão); `Protocol` (JSON); `OutboxPublisher` (ordem, 5/s, markSent) | MQTT |
| `core` | `BlockingQueue<T>` (PI mutex + cond monotónica, capacidade fixa); `Event`, `CommCommand` (`std::variant` de structs pequenos); `HealthMonitor`; `Watchdog` | Infraestrutura |

- **Direção das dependências.**
  - `logic` não depende de nada.
  - `devices` e `outputs` dependem das interfaces de `hw`.
  - `OutboxPublisher` depende de `IMessageTransport`.
  - O `main` é a composition root: liga a implementação real ou a simulada.
- **Refinamentos face às propostas anteriores.**
  - `DoorSensor`, `SmokeSensor` e o botão passam a uma só classe, `DebouncedInput`, configurada por parâmetros (amostras de confirmação, nível ativo, warm-up).
  - O estado de ligação à rede é um campo do `AlarmLogic`, sem classe própria, porque depois da DEC-001 não tem regras próprias.
  - A energia mantém classe própria (`PowerFsm`), porque tem regras.
- **Sem interface, de propósito (trade-offs declarados).**
  - `EventStore` e `PinStore`: o SQLite em memória serve de dublê de teste.
  - `AlarmOutputs`: as suas dependências já são abstratas.
  - `PinHasher`: é um cálculo; testa-se com a implementação real.
  - `Watchdog`: só o `main` o usa; na simulação usa-se uma versão vazia, escolhida na compilação. O `HealthMonitor` é puro e testável.
  - FSMs: `switch` sobre enum, sem State pattern.
- **Regras para o sistema embebido.**
  - Cada objeto de domínio pertence a uma única thread; só as duas filas são partilhadas.
  - Não há alocação dinâmica nos ciclos de tempo real.
- **SOLID.**
  - S: as threads só fazem agendamento; acabam as god classes; a arbitragem das saídas fica num só sítio.
  - O: aplica-se a sensores, tons, hardware real vs simulado e broker. Não se aplica, de propósito, às regras do alarme.
  - L: contrato do `WorkerThread` e substituibilidade real/simulado, verificável com os mesmos testes.
  - I: interfaces por capacidade; storage dividida por cliente; acabam as structs-união.
  - D: `logic` independente de tudo; trade-offs declarados para `EventStore`, `PinHasher` e `Watchdog`.
- **Riscos e consequências.**
  - O número de classes fica próximo do atual (~35). O que diminui são as threads, as filas, as dependências entre threads e as classes com mais de uma razão para mudar.
  - A apresentação passa a ter três vistas: concorrência; class diagram simplificado por package (~12 classes); diagrama completo no anexo.
  - Os nomes mudam, o que choca com a regra "keep the names" do Guide §0.1 (CON-08).
- **Alterações pendentes de execução:**
  - class diagram (slides 44–46) e package diagram (slide 43);
  - slides 68 e 70 (via DEC-009);
  - coluna "Responsible thread" das tabelas de interface (slides 16–23);
  - fluxogramas, que passam a referir os métodos das novas classes;
  - Guide §3.2, §5.4–5.5, §5.7 e §7;
  - o documento "Class Diagrams for Slides (PlantUML)" fica substituído;
  - classes que desaparecem ou mudam de nome: `IRSensorsThread`, `MQ2SensorThread`, `OutputThread`, `StorageThread`, `NetworkMQTTThread`, `SupervisorThread`, `OutputCmd`, `StorageCmd`, `NetCmd`, `IEventSink`, `LinkMonitor`.

### DEC-004 — Supervisão e tratamento de erros

- **Problema.** A DEC-001 fixou o essencial (watchdog sem reinício de threads, um FAULT por componente, base de dados recriada no arranque). Ficavam por definir três coisas:
  - o estado do sistema depois de um reinício;
  - o que fazer com erros inesperados;
  - o que continua a funcionar quando cada componente falha.
  
  O slide 86 tinha incoerências (CON-06), e o slide de error handling é obrigatório no curso.
- **Solução anterior.**
  - Arranque sempre DISARMED (Análise slide 15; Guide G10).
  - Supervisor a reiniciar threads.
  - FAULT para 6 componentes, com novas tentativas.
  - Base de dados com backup, restauro e fila em RAM.
  - "IR or MQ-2 stuck" no slide 86, embora não exista deteção de sensores presos.
- **Decisões.**

| Sub-decisão | Escolha |
|---|---|
| 4.1 Estado depois de um reinício | **b — repõe o último estado armado/desarmado** |
| 4.2 Erros inesperados | **b — registar e deixar o watchdog reiniciar o Pi** |
| 4.3 Modo degradado | **FAULT só para componentes de hardware + FAULT(DATABASE) no arranque; BD em funcionamento → só syslog** |
| 4.4 Aviso local de falha do MQ-2 | **a — só o alerta FAULT (sem chirp)** |
| 4.5 Parâmetros | **Valores por omissão aceites** |

- **4.1 — Reposição do estado.**
  - No arranque, o `main` lê o último ARMED ou DISARMED no EVENT_LOG (ordenado por `id`, sem depender do relógio) e injeta-o no `AlarmLogic`.
  - ARMED, ENTRY_DELAY e INTRUSION_ALARM regressam como ARMED; os restantes como DISARMED. Se a base de dados tiver sido recriada, o arranque é DISARMED.
  - A reposição não gera eventos.
  - O `DebouncedInput` usa a primeira leitura como referência: uma porta já aberta no arranque não dispara.
  - **Diverge da Análise** ("starts disarmed", slide 15); é preciso uma linha de justificação na apresentação.
  - Estimativa: ~30–40 linhas e 3–4 testes.
- **4.2 — Erros inesperados.**
  - Uma exceção ou um estado impossível numa thread é registado; a thread deixa de enviar heartbeats; o watchdog reinicia o Pi, que regressa no estado reposto pela 4.1.
  - A política fica num só sítio, a `WorkerThread`.
  - Falhas esperadas (erros de leitura de um dispositivo) seguem o modo degradado (4.3), não provocam reinício.

| Falha | Resposta | O que continua a funcionar |
|---|---|---|
| IR1 / IR2 | FAULT(IR1/IR2); o sensor fica ignorado até reiniciar | Tudo o resto |
| MQ-2 | FAULT(MQ2) | Tudo exceto a deteção de incêndio |
| Teclado / botão | FAULT(KEYPAD) | O desarme remoto. **Limitação:** um alarme de incêndio não pode ser silenciado até reiniciar |
| Driver PWM | FAULT(SPEAKER) (inclui o antigo PWM_DRIVER) | O LED RGB e os alertas |
| INA219 | FAULT(POWER_MONITOR) | Tudo exceto a deteção de falha da rede elétrica, a bateria fraca e o estado seguro |
| BD no arranque | Base nova + PIN por omissão + FAULT(DATABASE) | Tudo |
| BD em funcionamento | Só syslog; os alertas desse período perdem-se, mesmo com ligação (limitação declarada de falha dupla) | O alarme local e o PIN em RAM |
| Broker / Wi-Fi | Não é falha: reconexão sem fim (backoff de 1 a 60 s) + outbox + RGB (requisito) | Todo o funcionamento local |
| Thread presa | Heartbeat antigo → o watchdog reinicia o Pi | — |

- **Regras do modo degradado.**
  - Cada FAULT é enviado uma vez por componente em cada arranque, sem novas tentativas.
  - Componentes com FAULT: IR1, IR2, MQ2, KEYPAD, SPEAKER, POWER_MONITOR e DATABASE (este só no arranque).
  - Cada linha GPIO é pedida em separado, para que a falha de uma não arraste as outras.
  - Estimativa: ~95–110 linhas e 8–10 testes, quase todos no portátil com falhas simuladas.
- **4.5 — Parâmetros.**
  - Timeout de heartbeat: LocalInput 1 s; SafetySensors 1 s; PowerMonitor 3 s; Controller 3 s; Comm 10 s.
  - Watchdog: 15 s (o máximo do bcm2835), desativado numa paragem limpa (magic close).
- **SOLID.**
  - D preserva-se: o estado inicial é injetado pelo `main` e o `AlarmLogic` continua puro.
  - L reforça-se: a política de erros vive só na `WorkerThread` e é igual para todas as subclasses.
  - S e D preservam-se: os dispositivos só reportam falhas como eventos, e a regra "uma vez por componente" vive no `AlarmLogic`.
  - Sem abstrações novas: não se cria um `IErrorHandler`.
- **Riscos e consequências aceites.**
  - Janela de ~1–2 min sem proteção completa durante um reinício (watchdog + arranque + aquecimento do MQ-2), a medir.
  - Se a base de dados falhar em funcionamento, os alertas desse período perdem-se.
  - Com o teclado avariado, um alarme de incêndio não pode ser silenciado até reiniciar.
  - Com o MQ-2 avariado e sem rede, a falha não é visível localmente (4.4 a).
  - O contador do lockout volta a zero com o reinício.
  - O `event_id` volta a 1 quando a base de dados é recriada; resolvido na DEC-006 (6.2 a).
- **Alterações pendentes de execução:**
  - slide 86 refeito a partir da tabela do modo degradado (resolve a CON-06);
  - slide 53 (arranque: reposição do estado e recriação da base de dados);
  - slide 64 (ciclo do `main`);
  - slide 33 (valores do FAULT: KEYPAD e POWER_MONITOR entram; PWM_DRIVER junta-se a SPEAKER);
  - slide 49 (transição inicial da FSM de intrusão com a escolha do estado reposto);
  - nota de justificação da diferença face à Análise (slide 15 do deck A);
  - testes novos: reposição do estado; exceção → reset pelo watchdog; um teste por FAULT; base de dados corrompida; primeiro arranque;
  - Guide §6.8, §7.1 e §14; Decisions Record (falhas e base de dados); documento da GUI (textos do alerta "Device fault").

### DEC-005a — Limiar de bateria fraca em tensão

- **Problema.** Com M1, a percentagem de bateria deixou de ser apresentada, por isso o limiar "≤ 30 %" deixou de fazer sentido.
- **Decisão.** **LOW_BATTERY quando a tensão está ≤ 6,7 V durante 3 amostras seguidas, só quando o sistema está a bateria.** O limiar crítico mantém-se em 6,4 V.
- **Risco.** "30 % ≈ 6,7 V" vinha de uma conversão linear, e a descarga real de Li-ion não é linear. Convém medir no teste de autonomia o tempo entre 6,7 V e 6,4 V.
- **Alterações pendentes de execução:**
  - slide 23 (limiar e linhas de percentagem);
  - Guide §4.5, §7.5 e §15;
  - Decisions Record (regra Low battery).

### DEC-005b — Restante gestão de energia

- **Problema.** Depois da DEC-001 (M1 e encerramento simplificado) e da DEC-005a (6,7 V), ficavam quatro pontos por definir:
  - o conteúdo dos alertas de energia;
  - o estado de energia no arranque;
  - o que acontece abaixo de 6,4 V;
  - como o broker fica a saber que o Pi parou.
- **Solução anterior.**
  - Percentagem de bateria nos alertas.
  - Alertas de energia no arranque.
  - `poweroff` coordenado com a publicação de "offline" com motivo e DISCONNECT 0x00.
- **Decisões.**

| Sub-decisão | Escolha |
|---|---|
| 5b.1 `details` dos alertas de energia | **c — tensão só no LOW_BATTERY** (ex.: "6.68 V"); POWER_LOST e POWER_RESTORED com `details` null |
| 5b.2 Estado de energia no arranque | **Confirmado:** as três primeiras amostras decidem em silêncio (sem alerta) se está na rede elétrica ou na bateria; até lá assume-se rede elétrica |
| 5b.3 Abaixo de 6,4 V | **b — estado seguro, sem desligar** (substitui o `poweroff` da DEC-001) |
| 5b.4 Saída do broker numa paragem limpa | **a — DISCONNECT com o código 0x04** ("Disconnect with Will Message") |

- **5b.3 — Estado seguro.**
  - Abaixo de 6,4 V (3 amostras, a bateria): alerta SHUTDOWN na outbox, a base de dados fecha-se e faz-se `sync`.
  - O sistema continua a funcionar: o alarme local protege até o HAT cortar a energia. Os eventos seguintes vão só para o syslog, como na falha da base de dados em funcionamento (4.3).
  - Quando a rede elétrica regressa, o HAT volta a dar energia, o Pi arranca sozinho e a 4.1 repõe o estado.
  - **Porquê:** com `poweroff`, se a rede elétrica voltasse antes de a bateria se esgotar, o Pi podia ficar desligado indefinidamente (issue 7 do Work Log).
  - **Condição:** confirmar no teste de descarga que o HAT corta a saída na descarga e a repõe quando o carregador volta. Se não repuser, volta-se ao `poweroff`, o que exige uma nova decisão.
  - Estimativa: ~15–20 linhas (reaproveita o caminho da 4.3).
- **5b.4 — DISCONNECT 0x04.**
  - Numa paragem limpa (SIGTERM, manutenção), o broker publica logo o Last Will "offline".
  - Na paragem por bateria, a energia cai sem DISCONNECT, e o Last Will chega pelo keep-alive (~45 s).
  - A verificar: suporte no Paho C++ e no EMQX (norma MQTT 5).
  - Estimativa: ~2–5 linhas e 1 teste com o Mosquitto.
- **SOLID.** Efeito neutro:
  - a `PowerFsm` produz a ação "entrar em estado seguro" e a thread Comm fecha o `EventStore`;
  - o DISCONNECT 0x04 fica dentro do `MqttLink`;
  - não há classes nem interfaces novas.
- **Riscos e consequências.**
  - Em estado seguro, as baterias descarregam até à proteção do HAT.
  - A app deixa de poder mostrar o motivo do "offline". O alerta LOW_BATTERY anterior e o SHUTDOWN (entregue quando houver ligação, eventualmente depois do arranque seguinte) dão esse contexto.
- **Alterações pendentes de execução:**
  - slide 23 (interface de energia);
  - slide 32 (alertas de energia: `details` e gatilhos);
  - slide 30 (sai a mensagem "Device Offline (clean)" com motivo; entra o DISCONNECT 0x04);
  - slide 40 (a sequência de encerramento por bateria fraca é substituída pelo estado seguro);
  - slide 51 (o estado SHUTDOWN da FSM de energia passa a "estado seguro"; arranque silencioso);
  - slide 61 (fluxograma do PowerMonitor);
  - testes do slide 81 (bateria fraca, estado seguro, autonomia);
  - Guide §4.5, §6.5, §6.9, §7.5, §10.2 e §10.4; documento da GUI (textos); Decisions Record (regras de Low battery e Shutdown).

### DEC-006 — Protocolo remoto e âmbito da app

- **Problema.** Duas questões vinham de decisões anteriores:
  - os eventos de registo gerados com o Pi offline perdiam-se, porque o `get_events` foi adiado;
  - o `event_id` volta a 1 quando a base de dados é recriada.
  
  Além disso, faltava confirmar o âmbito da app e simplificar a regra de reconexão.
- **Solução anterior.**
  - Só os alertas passavam pela outbox (tabela PENDING_NOTIFICATION); os eventos de registo eram publicados uma vez e perdiam-se sem ligação.
  - A app eliminava duplicados e ordenava a lista pelo `event_id`.
  - Havia duas regras de reconexão: backoff de 1 a 60 s, e erros de autenticação a cada 5 min.
- **Decisões.**

| Sub-decisão | Escolha |
|---|---|
| 6.1 Eventos de registo com o Pi offline | **b' — todos os eventos pela outbox, que passa a ser uma coluna do EVENT_LOG** |
| 6.2 `event_id` recomeça com a base de dados recriada | **a — a app elimina duplicados pela mensagem completa e ordena por ordem de chegada** |
| 6.3 Regra de reconexão | **b — uma só regra: backoff de 1 a 60 s para todas as falhas, com registo uma vez por episódio** |
| 6.4 Âmbito da app | **a — âmbito reduzido** |

- **6.1 b' — Outbox no EVENT_LOG.**
  - Esquema: EVENT_LOG (`id`, `event_type`, `timestamp`, `details`, `status` PENDING/SENT, `sent_at`) e PIN_CONFIG. **A tabela PENDING_NOTIFICATION desaparece**: a base de dados passa de 3 para 2 tabelas.
  - Todos os eventos, alertas e de registo, são inseridos como PENDING, publicados por ordem do `id` (≤ 5/s) e marcados SENT quando chega o PUBACK.
  - O `Protocol` constrói o JSON a partir da linha e escolhe o tópico pelo tipo de evento (`alert` ou `event`). As notificações no telemóvel continuam só no tópico `alert`.
  - **Diverge da Análise**, cujo ERD tinha 3 tabelas; é preciso uma linha de justificação na apresentação.
  - Estimativa: menos ~20–30 linhas face ao desenho anterior (um só INSERT, sem tabela ligada).
- **6.2 a — Duplicados na app.**
  - Os duplicados do QoS 1 são cópias exatas; um evento novo com um `id` repetido tem outro timestamp ou tipo.
  - Com a 6.1 b', todos os eventos saem pela ordem do `id`, por isso a ordem de chegada é a ordem dos eventos.
  - O protocolo não muda. Estimativa: ~10–15 linhas em Dart.
- **6.3 b — Reconexão.** Menos ~5–10 linhas e 1 teste. Com credenciais erradas faz-se uma tentativa por minuto, o que não causa problemas ao broker.
- **6.4 a — Âmbito da app.**
  - Android, Flutter, um só ecrã, tema claro, sem login.
  - Cartão de estado (incêndio > intrusão > incêndio silenciado > armado/desarmado), linha de energia "Rede elétrica"/"Bateria" sem percentagem, e indicação online/offline pelo Last Will.
  - Botões Armar, Desarmar e Mudar PIN, com diálogos, todas as respostas do Pi e timeout de 32 s.
  - Lista dos últimos 20 eventos (alertas e registo), por ordem de chegada.
  - Notificações só para o tópico `alert`, através de um foreground service.
  - Fica de fora: sincronização de histórico, resumo de reconexão, iOS, várias casas.
- **SOLID.**
  - S melhora: o `OutboxPublisher` tem uma só política de envio, o `Protocol` decide o tópico e o `EventStore` fica com menos métodos.
  - O `MqttLink` fica com menos ramos.
  - A 6.2 não toca no Pi.
  - Não há abstrações novas.
- **Riscos e consequências.**
  - O ERD muda face à Análise (de 3 para 2 tabelas).
  - O foreground service com MQTT num isolate continua a ser a parte mais arriscada da app; convém fazer um protótipo cedo.
  - Mais escritas na outbox (todos os eventos), em número desprezável.
- **Alterações pendentes de execução:**
  - slides 26–27 (armazenamento e ERD);
  - slides 29–30 e 34–36 (especificação de mensagens);
  - slides 37–39 (sequências MQTT);
  - slide 41 (pilha de comunicação: nome da thread);
  - slides 72–75 (GUI) e testes 82–84;
  - Guide §9, §10 e §13; Decisions Record (outbox, eventos de registo, reconexão, ERD); documento da GUI; ERD nas instruções do Project.

### DEC-007 — Hardware em aberto

- **Problema.** Três pontos de hardware estavam por fechar ou eram contraditórios entre documentos:
  - **LED RGB.** O Guide (§4.1, §4.2, §7.7) e o Hardware Context (§11, §12) ligam os cátodos diretamente ao GPIO (LOW = aceso), com o ânodo a 5 V. O deck (slides 11–12) já mostra um transístor NPN por cátodo (CON-03).
  - **Sensores IR.** O tipo de saída (open-collector ou push-pull a 5 V) não está confirmado. O pull-up a 3,3 V ligado diretamente à saída (slide 12) só é seguro se a saída for open-collector; o divisor resistivo do Hardware Context só funciona se for push-pull.
  - **Montagem física.** Nenhuma fonte decidia como o sistema é montado e instalado. O slide 13 mostra uma caixa, mas a decisão não estava registada.
- **Decisões.**

| Sub-decisão | Escolha |
|---|---|
| 7.1 Acionamento do LED RGB | **b — um NPN low-side por cátodo; ânodo a 5 V; GPIO HIGH = aceso** |
| 7.2 Interface dos sensores IR | **b — interface universal: díodo Schottky em série + pull-up externo de 10 kΩ a 3,3 V** |
| 7.3 Montagem física | **b — breadboard no bring-up; depois placa de interface em protoboard, dentro de uma caixa** |

- **7.1 b — Um NPN por cátodo.**
  - Circuito: GPIO → 1 kΩ → base do BC337 (ou 2N2222A); 10 kΩ entre a base e GND; coletor → resistência do canal → cátodo; emissor → GND. Ânodo comum a +5 V.
  - As resistências dos canais mantêm-se: 390 Ω (R), 220 Ω (G) e 220 Ω (B), ≈ 7–8 mA por canal. Cada base consome ≈ 2,6 mA do GPIO.
  - Porquê:
    - nenhum nó a 5 V toca num GPIO, que não tolera 5 V;
    - o canal vermelho apaga por completo (com a ligação direta ficavam 1,7 V no díodo, perto do V_F mínimo de 1,8 V);
    - o pull-down de 10 kΩ mantém o LED apagado enquanto o GPIO flutua durante o arranque;
    - a lógica passa a ser ativa a HIGH, como nas restantes saídas.
  - Alternativa principal rejeitada: manter a ligação direta ao GPIO (Guide e Hardware Context). Poupa os transístores, mas tem os três problemas acima.
  - **Resolve a CON-03.** O deck já mostra este circuito (slides 11–12). Os documentos desatualizados são o Guide e o Hardware Context.
- **7.2 b — Interface universal dos IR.**
  - Circuito: saída do sensor → cátodo de um Schottky (BAT85, preferido pela fuga baixa; o 1N5817 também serve); ânodo → GPIO; pull-up externo de 10 kΩ do GPIO para 3,3 V. O pull-up interno do GPIO fica desativado.
  - Funciona com os dois tipos de saída:
    - saída a LOW: o díodo conduz e o GPIO lê ≈ 0,3–0,5 V, abaixo do V_IL máximo de 0,8 V;
    - saída a HIGH (5 V ou aberta): o díodo bloqueia e o pull-up dá 3,3 V;
    - nunca entram 5 V no GPIO.
  - A polaridade não muda: porta fechada = LOW; porta aberta = HIGH.
  - Porquê: o esquema deixa de depender de uma medição que só pode ser feita quando houver componentes.
  - Continua a ser preciso medir na receção (Hardware Context §17): tipo de saída, nível LOW no GPIO e distância de deteção face à porta.
- **7.3 b — Montagem em duas fases.**
  - **Bring-up:** breadboard, um circuito de cada vez, para testar cada periférico e o driver.
  - **Demonstração:** uma placa de interface em protoboard com os circuitos de apoio (driver do altifalante, 3 NPN do RGB, divisor do MQ-2, díodos e pull-ups dos IR) e terminais para os cabos dos sensores. Tudo fica numa caixa, com o teclado, o botão de painel (Ref. 1188), os LEDs e o altifalante no painel frontal.
  - **Instalação:** IR no aro da porta ou janela; MQ-2 numa posição alta (parede alta ou teto), porque o fumo sobe; cabos dos sensores até à caixa.
  - Porquê: a breadboard sozinha é frágil para a demonstração (um contacto solto pode dar um falso alarme ou esconder um verdadeiro), e uma PCB feita por medida custa mais tempo do que o projeto tem.
  - **Caixa (decidido a 9 nov):** uma caixa de plástico comercial, comprada numa loja (bazar), para simplificar o trabalho. Leva furos para o teclado, o botão, os LEDs e o altifalante, e ranhuras de ventilação para o Pi. Antes de a comprar, confirmar que cabem o Pi com o HAT empilhado e a placa de interface.
- **Lista consolidada a comprar.** Componentes principais em falta (Hardware Context §16) e componentes de apoio, com contagens tiradas do esquema do slide 12 e das decisões 7.1 e 7.2.

| Item | Qtd. | Uso | Reserva sugerida |
|---|---|---|---|
| UPS HAT (Botnroll Ref. 4106, com carregador de 8,4 V) | 1 | Alimentação e monitor INA219 | — |
| Baterias Li-ion 18650, 2600 mAh (Ref. BAT03005) | 2 | Bateria do HAT (2S) | — |
| Teclado 3×4 (Ref. 434) | 1 | Entrada do PIN | — |
| Botão de pressão (Ref. 716 para a breadboard, Ref. 1188 para o painel) | 1 + 1 | Armar localmente | — |
| Módulo MQ-2 (Ref. 939) | 1 | Fumo/gás | — |
| NPN BC337 (ou 2N2222A) | 4 | 3 × RGB + altifalante | +2 |
| Díodo 1N4148 | 1 | Flyback do altifalante | +1 |
| Schottky BAT85 (ou 1N5817) | 2 | IR1, IR2 | +1 |
| Resistência 220 Ω | 3 | LED vermelho; RGB G e B | — |
| Resistência 390 Ω | 1 | RGB R | — |
| Resistência 470 Ω | 1 | Base do NPN do altifalante | — |
| Resistência 1 kΩ | 3 | Bases dos NPN do RGB | — |
| Resistência 10 kΩ | 7 | 4 pull-downs de base, 2 pull-ups dos IR, R1 do divisor do MQ-2 | — |
| Resistência 20 kΩ | 1 | R2 do divisor do MQ-2 | — |
| Resistência 33 Ω, 0,5 W | 1 | Série do altifalante | — |
| Protoboard (placa perfurada) | 1 | Placa de interface | — |
| Pin headers, terminais de parafuso, jumpers Dupont, cabo multifilar | 1 conjunto | Ligações e cabos dos sensores | — |
| Dissipadores para o Pi | 1 conjunto | O HAT tapa a placa | — |
| Caixa de plástico comercial | 1 | Comprada numa loja (bazar); confirmar as dimensões | — |

Um kit sortido de resistências de 1/4 W cobre todos os valores, exceto a de 0,5 W. **As compras estão no caminho crítico** de todos os testes em hardware (driver, sensores, energia). O trabalho no portátil (lógica, simulação, MQTT) não depende delas. Como os sensores IR já estão disponíveis, a medição do tipo de saída e do nível LOW (7.2) pode ser feita já, com o 5 V do Pi e um multímetro.
- **SOLID.** Efeito neutro:
  - o nível ativo de cada linha é um parâmetro de `GpioLines` e `DebouncedInput`, por isso passar o RGB de ativo a LOW para ativo a HIGH é configuração, não código (exemplo concreto de O);
  - o díodo dos IR é transparente para o software;
  - não há classes nem interfaces novas.
- **Riscos e consequências.**
  - O Schottky soma ≈ 0,25–0,3 V ao nível LOW do sensor. Se o LOW do sensor for alto (acima de ≈ 0,5 V), a margem face aos 0,8 V diminui; a medir na receção.
  - O RGB passa a ter mais 3 transístores e 6 resistências.
  - Sem as compras, nenhum teste em hardware começa. Em particular, até o UPS HAT chegar não é possível verificar a condição da 5b.3 (o HAT corta e repõe a saída); se falhar, a 5b.3 é reaberta.
- **Alterações pendentes de execução:**
  - slides 9 (interface do IR), 11 (RGB: ânodo a 5 V e pull-down de 10 kΩ), 12 (Schottky nos IR, pull-up do lado do GPIO), 13 (legenda com as fases de montagem e a localização dos sensores) e 14 (linhas dos IR e do RGB);
  - Guide §4.1, §4.2, §4.3, §7.7 e §12.2;
  - Hardware Context §11, §12, §15, §16 e §17;
  - Decisions Record D-46.

### DEC-008 — Estratégia de testes e testes de aceitação

- **Problema.**
  - Os slides 76–85 têm cerca de 75 testes em 10 slides, com 3 colunas (Test / Action / Expected Result), sem ID, objetivo, nível nem método, e sem ligação aos requisitos.
  - Cerca de 12 testes ficaram inválidos com as decisões anteriores, e faltavam testes para o que essas decisões criaram (reposição do estado, FAULT, estado seguro, DISCONNECT 0x04, glitch no IR, outbox para todos os eventos).
  - O slide 85 dá alvos de tempo real sem dizer como se medem.
  - O Revision Plan propunha chegar a cerca de 110 testes, o contrário de simplificar.
- **Solução anterior.** Um catálogo único de testes, implicitamente todos no Pi. O Guide §16 e o Revision Plan acrescentavam cerca de 35 linhas e uma coluna "Method".
- **Alternativas.**
  - 8.1 Níveis: a — sem níveis (tudo no Pi); b — 3 níveis; c — 4 níveis (com um L4 de sistema separado).
  - 8.2 Apresentação: A — corrigir os 10 slides atuais; B — estratégia + cerca de 24 testes de aceitação; C — catálogo completo (cerca de 110 testes).
  - 8.3 Medição de tempo real: a — só software; b — software + medição externa; c — só medição externa.
- **Recomendação.** 8.1 b, 8.2 B, 8.3 b.
- **Decisão.**

| Sub-decisão | Escolha |
|---|---|
| 8.1 Níveis de teste | **b — L1 portátil (unitários), L2 portátil (integração), L3 Raspberry Pi** |
| 8.2 Testes na apresentação | **B — 1 slide de estratégia + 24 testes de aceitação em cerca de 4 slides; o catálogo completo fica no anexo ou no relatório** |
| 8.3 Medição de tempo real | **b — software + medição externa, mas só como nota.** A apresentação mostra os alvos e as condições de carga, sem referir equipamento nem testes específicos no laboratório |

- **8.1 b — Níveis.**
  - **L1, portátil, unitários** (GoogleTest + ctest, Guide §12.6): `AlarmLogic` e FSMs com o tempo injetado; `DebouncedInput`, `KeypadScanner`, `PinEntry` e `PowerSupplyMonitor` sobre hardware simulado; `EventStore` e `PinStore` com SQLite em memória; `Protocol`; `BlockingQueue`.
  - **L2, portátil, integração:** binário `VIGIL_SIM`, Mosquitto, `mosquitto_sub` e a app.
  - **L3, Raspberry Pi:** hardware real, driver `vigil_pwm`, energia, tempo real sob carga, EMQX e a app no telemóvel.
  - O L1 e o L2 podem começar já; o L3 depende das compras.
- **8.2 B — Testes de aceitação.**
  - Colunas: ID · Objective · Stimulus · Expected result · Level · Req.
  - Regra: cada requisito da Análise é coberto por pelo menos 1 teste de aceitação.
  - O texto em inglês para os slides está no documento de alterações à apresentação.

| ID | Teste | Nível | Requisitos da Análise (nomes do deck A, slides 5–7) |
|---|---|---|---|
| AT-01 | Armar localmente com exit delay | L3 | Flexible Control; Entry/Exit Delay; Visual Status |
| AT-02 | Desarmar durante o entry delay | L3 | Entry/Exit Delay; Flexible Control |
| AT-03 | Alarme de intrusão (alerta no telemóvel em ≤ 5 s) | L3 | Dual Detection; Distinct Alarms; Instant Alerts |
| AT-04 | PIN errado → lockout | L1 + L3 | Security; Flexible Control |
| AT-05 | Incêndio em qualquer estado, com tom distinto | L3 | Dual Detection; Distinct Alarms |
| AT-06 | Período de silêncio e reativação | L1 + L3 | Fire Alarm Silence Period |
| AT-07 | Armar e desarmar remotamente; desarme rejeitado durante incêndio | L2 + L3 | Flexible Control |
| AT-08 | Mudar o PIN pela app | L2 | PIN Management; Security |
| AT-09 | Glitch no IR ignorado | L1 | Dual Detection |
| AT-10 | Aquecimento do MQ-2 sem falso alarme | L3 | Dual Detection |
| AT-11 | Falha e regresso da rede elétrica | L3 | Power Resilience; Instant Alerts; Visual Status; Availability |
| AT-12 | Bateria fraca, estado seguro e rearranque | L3 | Power Resilience |
| AT-13 | Autonomia | L3 | Power Resilience |
| AT-14 | Eventos gerados offline entregues por ordem | L2 + L3 | Pending Notification Handling; Connectivity Monitoring; Event Logging |
| AT-15 | Deteção de offline (Last Will e DISCONNECT 0x04) | L2 + L3 | Instant Alerts |
| AT-16 | Canal seguro (TLS, ACL, comando expirado) | L2 + L3 | Security |
| AT-17 | Estado reposto depois de um reinício | L2 + L3 | Availability |
| AT-18 | Recuperação pelo watchdog | L3 | Availability |
| AT-19 | FAULT de um componente | L2 | Availability |
| AT-20 | Base de dados corrompida no arranque | L2 + L3 | Event Logging; Availability |
| AT-21 | Driver PWM próprio | L3 | Distinct Alarms; Visual Status |
| AT-22 | Latência do alarme sob carga | L3 | Real-Time Responsiveness |
| AT-23 | Resposta do teclado e ciclos periódicos sob carga | L3 | Real-Time Responsiveness |
| AT-24 | Estabilidade em 24 h | L3 | Availability |

- **Catálogo completo.** Os restantes testes (unitários e de integração) ficam no anexo da apresentação ou no relatório (decide-se na Fase 4) e no Guide §16, corrigidos pelas decisões.
- **8.3 b — Método de medição (nota; não entra na apresentação).**
  - Software: timestamps `CLOCK_MONOTONIC` na leitura que gera o evento e no `write()` para o driver; `cyclictest` para a latência do sistema; `stress-ng` para a carga.
  - Externa: osciloscópio ou analisador lógico, com um canal no pino de entrada (IR ou MQ-2) e outro no GPIO18; pelo menos 50 amostras, com mínimo, média e máximo. Não precisa de código extra.
  - Na apresentação ficam só os alvos (≤ 1 s do sensor ao alarme local, ≤ 100 ms da tecla ao beep, ≤ 5 s até à notificação) e as condições de carga do estímulo.
  - O equipamento de laboratório não está confirmado. Se não houver, fica só a medição por software, com a limitação declarada no relatório.
- **SOLID.** Os testes são a evidência das afirmações SOLID:
  - D: o `AlarmLogic` testa-se sem hardware nem relógio do sistema (L1);
  - L: os mesmos testes de contrato correm contra as implementações simuladas (L1/L2) e reais (L3) de `IGpioInput`, `IPwm` e `IPowerSensor`;
  - S: cada classe testa-se isolada.
  
  Não há abstrações novas: usam-se as simulações já previstas e o SQLite em memória, sem criar mocks para cada classe.
- **Riscos e consequências.**
  - O L3 depende das compras.
  - O AT-19 só é viável no L2, com a falha simulada, porque no Pi não há forma simples de provocar uma falha de leitura do libgpiod. Fica declarado.
  - O AT-18 precisa de uma forma de bloquear uma thread numa build de teste (por exemplo, um sinal só ativo nessa build). São poucas linhas e não afetam a build de produção.
  - Os testes de contrato exigem uma simulação fiel ao hardware (já era um dos motivos da B').
  - O deck passa de 10 slides de testes para 5.
- **Alterações pendentes de execução:**
  - slides 76–85, substituídos por 1 slide de estratégia + cerca de 4 slides de testes de aceitação;
  - catálogo completo no anexo ou no relatório, corrigido pelas decisões;
  - Guide §16 (plano de testes) e §19 (os marcos referem testes por número de slide);
  - as "≈ 35 novas linhas" e a coluna "Method" do Revision Plan ficam substituídas por esta decisão.

### DEC-009 — Mapa de evidência SOLID

- **Problema.**
  - O slide 70 tinha afirmações que deixaram de ser verdade com as decisões anteriores: "Each thread has one job", "The 8 WorkerThread subclasses used by SupervisorThread", `IEventSink` e "New tone = row in the driver pattern table".
  - "New event = EventType value" já era falsa: um evento novo obriga a editar o `switch` do `AlarmLogic`, ou seja, é uma modificação e não uma extensão.
  - O slide 68 tinha 4 padrões que deixaram de existir: Command, Active Object, Observer (`IEventSink`) e Supervisor a reiniciar threads.
  - Nenhum dos dois slides dizia como verificar cada afirmação.
- **Solução anterior.** Slide 68: tabela de 10 padrões (Pattern / Where / Problem it solves). Slide 70: tabela SOLID (Principle / How it is applied / Where in the design).
- **Alternativas.**
  - 9.1 Slide SOLID: A — corrigir só o texto; B — mapa de evidência (Principle · Where · Evidence); C — um slide por princípio.
  - 9.2 Slide de padrões: a — remover; b — manter só os padrões que existem; c — juntar com o slide SOLID.
  - 9.3 Onde o SOLID não se aplica de propósito: a — mostrar no slide; b — não apresentar.
- **Recomendação.** 9.1 B, 9.2 b, 9.3 a.
- **Decisão.**

| Sub-decisão | Escolha |
|---|---|
| 9.1 Slide SOLID | **B — mapa de evidência, mais notas SOLID curtas ao longo da apresentação** |
| 9.2 Slide de padrões | **b — só os 8 padrões que existem depois das decisões** |
| 9.3 Onde o SOLID não se aplica | **Não apresentar.** Os motivos ficam registados na DEC-003b, para responder se o professor perguntar |

- **9.1 — Mapa e notas.**
  - Slide 70 com as colunas Principle · Where · Evidence. Cada evidência é um teste da DEC-008 ou um cenário de alteração concreto. O texto em inglês está no documento de alterações à apresentação.
  - Notas: uma linha curta ("SOLID – O: …"), no máximo uma por slide, nos slides onde o princípio se vê: package/class diagram, interface do IR, task overview, armazenamento/Comm, driver e estratégia de testes.
  - Cada nota corresponde a uma linha do mapa; o slide 70 funciona como resumo das notas.
- **9.2 — Padrões.** Saem Command, Active Object, Observer e Supervisor. Entra Composition Root. Ficam Producer–Consumer, Monitor Object, Template Method, FSM pura, Adapter, Transactional Outbox e Watchdog.
- **9.3.** O mapa e as notas só afirmam onde o SOLID se aplica, e nenhuma afirmação pode ser falsa no código. Por isso não se diz que as regras do alarme são extensíveis.
- **SOLID.** Não muda a arquitetura. O mapa passa a servir de checklist para rever o código e entra no ficheiro de diretrizes (Fase 5).
- **Riscos e consequências.**
  - Se o código não cumprir o mapa, o professor pode confrontar os dois na avaliação final.
  - A pergunta "porque é que esta classe não tem interface?" responde-se oralmente, com os motivos da DEC-003b.
  - As notas têm de ficar coerentes com o mapa sempre que um slide muda.
- **Alterações pendentes de execução:**
  - slides 68 e 70 reescritos;
  - notas SOLID em cerca de 6 slides;
  - class diagram simplificado (slides 44–46) com todos os nomes que o mapa refere;
  - o ficheiro de diretrizes (Fase 5) inclui o mapa como checklist.

### DEC-010 — Sensor de intrusão e o requisito "movement" da Análise

- **Problema.**
  - A Análise pede "Dual Detection: Monitor movement and smoke/gas hazards", e a tabela de eventos indica "Motion Detection — Origin: PIR sensor" (deck A, slides 5 e 15).
  - O Design usa 2 sensores IR refletivos nas portas/janelas, sem justificação no deck. O evento continua a chamar-se `EVT_MOTION_DETECTED`.
  - Contradizia a confirmação de que todos os requisitos da Análise se mantêm.
- **Alternativas.**
  - A — reformular o requisito ("door/window opening") e mudar os nomes;
  - A' — concretizar o requisito sem o alterar;
  - B — acrescentar 1 PIR aos 2 IR;
  - C — trocar os IR por um PIR.
- **Recomendação.** Inicialmente B. Depois de o utilizador esclarecer que o objetivo é detetar o movimento da porta/janela e que ficam só os 2 IR, A'.
- **Decisão.** **A' — concretizar o requisito, só com os 2 IR.**
- **O que fica definido.**
  - O requisito da Análise mantém-se palavra por palavra.
  - No Design, "movimento" é o movimento da folha de uma porta ou janela vigiada. O IR fica no aro: folha à frente (fechada) = LOW; folha afastada (aberta) = HIGH. O evento é a transição fechada → aberta, confirmada em 2 leituras (DEC-003a).
  - O evento mantém o nome `EVT_MOTION_DETECTED` (origem IR1 ou IR2), para manter a ligação ao evento "Motion Detection" da Análise. Na app, o texto é "Intrusion – door/window moved (IR1)".
  - Linha de justificação no deck: "The Analysis assumed a PIR sensor. The design detects movement at the entry points: an IR reflective sensor on each door/window frame."
  - Razões para IR em vez de PIR (defesa oral):
    - deteta no ponto de entrada, antes de a pessoa estar dentro, o que é coerente com o entry delay;
    - não dispara com animais, fontes de calor ou sol;
    - responde em menos de 2 ms, sem tempo de bloqueio nem aquecimento.
- **Limitações declaradas.**
  - Só as 2 portas ou janelas vigiadas estão protegidas.
  - Armado com a porta aberta, o sistema só deteta depois de ela fechar e voltar a abrir.
- **Pergunta provável do professor:** "Porque não um interruptor magnético (reed switch)?" Resposta: hardware disponível e sem contacto; um reed switch ligava-se à mesma classe `DebouncedInput` sem alterar código.
- **SOLID.** Neutro; sem classes nem interfaces novas.
- **Consequências.** Sem hardware novo nem compras. Resolve a CON-19 ("object in front of IR").
- **Alterações pendentes de execução:**
  - slides 9 e 16, linha de justificação, textos da app (slides 73–75) e catálogo de testes;
  - documento da GUI (texto da notificação);
  - Hardware Context §1 e §2 (ainda dizem que na apresentação o sensor se chama "PIR");
  - Guide §4.1 (descrição do IR);
  - instruções do Project ("PIR", "movimento").

## 5. Questões em aberto ligadas a decisões aprovadas

| Questão | Decide-se em |
|---|---|
| Mock-ups da GUI sem % de bateria nem histórico sincronizado | Fase 4 (apresentação); listado no documento de alterações à apresentação |
| Dimensões da caixa comercial: cabem o Pi com o HAT e a placa de interface? | Antes de comprar a caixa (DEC-007, 7.3) |
| Tipo de saída e nível LOW real dos sensores IR | Medição já possível (os IR estão disponíveis); a interface da 7.2 serve para qualquer resultado |
| O UPS HAT corta e repõe a saída na descarga? | Primeiro teste quando o HAT chegar; se falhar, a 5b.3 é reaberta |
| Há osciloscópio ou analisador lógico no laboratório? | Antes das medições de tempo real (DEC-008, 8.3); sem ele, só a medição por software |

## 6. O que falta decidir (estado a 9 nov)

| Item | Tipo | O que bloqueia | Quando |
|---|---|---|---|
| DEC-011 — Precedência e destino dos documentos (CON-08: o Guide diz "do not redesign"; CON-01: instruções do Project obsoletas) | Decisão | A execução das alterações em bloco | Agora (proposta a 9 nov) |
| Autorização para executar as alterações (Guide, Decisions Record, Hardware Context, documento da GUI, deck) | Autorização | Começar a implementação com documentos coerentes | Depois da DEC-011 |
| Fase 5 — ficheiros de diretrizes (conteúdo e localização) | Aprovação | A regra do draw.io define como se redesenham os diagramas na Fase 4 | Antes de redesenhar diagramas (proposta: antes da Fase 4) |
| Fase 4 — estrutura da apresentação | Proposta + aprovação | — | Precisa da data e duração da apresentação e dos critérios de avaliação |

**Correções que não precisam de decisão** (executam-se na Fase 4): texto do slide 6 reutilizado do G4; CON-12 ("udhcpc" no slide 41); gralhas nos títulos dos slides 54, 59 e 71; Gantt (slide 87) e roadmap do Guide §19, que ainda referem funcionalidades removidas.

**Verificações que podem reabrir decisões:**

| Verificação | Decisão afetada |
|---|---|
| O UPS HAT corta e repõe a saída na descarga | 5b.3 |
| Tipo de saída e nível LOW dos IR | 7.2 |
| DISCONNECT 0x04 suportado pelo Paho C++ e pelo EMQX | 5b.4 |
| Tempo real entre 6,7 V e 6,4 V | 5a |
| Protótipo do foreground service com MQTT na app | 6.4 |
| Osciloscópio ou analisador lógico no laboratório | 8.3 |
| Dimensões da caixa | 7.3 |
