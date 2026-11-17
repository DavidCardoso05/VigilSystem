# VigilSystem — Guidelines

| Field | Value |
|---|---|
| Document | `VIGILSYSTEM_GUIDELINES.md` — how Claude and the team work on VigilSystem |
| Version | **1.1** — 2026-11-09 (v1.0 approved by David; v1.1 updates §7 for DEC-013, approved by David) |
| Date | 2026-11-09 |
| Scope | Project-specific rules. Reusable rules for every conversation live in David's personal preferences in Claude (global working rules) |
| Related | `VIGILSYSTEM_MASTER_CONTEXT.md` (what the system is and why); `CLAUDE.md` (entry point) |

---

## 1. Authority and reading order

1. `VIGILSYSTEM_MASTER_CONTEXT.md` is the single source of truth (precedence in its §0.1).
2. This document says **how** to work. If it seems to contradict the Master Context about the design, the Master Context wins and the contradiction is reported.
3. Read the Master Context §0 at the start of any task. Before proposing a design change, read §C, §E, §G and §N. Before changing any file, read §2 of this document.

---

## 2. Collaboration rules

### 2.1 Fundamental rule

> "YOU ARE NOT AUTHORIZED TO MODIFY, CREATE, DELETE, REPLACE, OR RESTRUCTURE ANY PROJECT FILE WITHOUT MY EXPLICIT APPROVAL." — David Cardoso

It covers code, architecture, hardware, requirements, presentations, documents, diagrams, configuration, tests, commits and installations. Reading files is allowed; writing is not, until David approves.

### 2.2 Procedure before any change

1. Analyse the current state.
2. Identify outdated information and contradictions.
3. List the files that would change.
4. Describe the intended changes and their consequences.
5. Give the alternatives (always including "keep the current solution"), the trade-offs and a recommendation.
6. Ask for explicit authorization, then execute **only** what was authorized.
7. Verify the result and summarise every file created or modified.

Approving one change is not approving others. **Approving a decision is not authorization to execute it.**

### 2.3 Decision format

Every decision is presented and recorded with: identifier (`DEC-0xx`), problem, current solution, alternatives (A/B/C, including keeping the current solution), technical analysis, recommendation, risks and consequences, SOLID Impact Analysis (for architectural decisions), affected components, the user's decision and its status (pending / approved / rejected / superseded). Features are classified **Keep / Simplify / Postpone / Remove**. Template: Appendix A.

### 2.4 Honesty and scope

- Distinguish facts, assumptions and proposals. Never invent explanations, decisions, measurements or test results.
- Challenge decisions with arguments when there is a reason to; do not simply agree.
- If a technical problem is found, report it; do not change the design silently and do not fix unrelated issues.
- A settled decision (Master Context §G) is reopened only for a new problem and only with authorization.
- Say clearly when information is missing, unverifiable or lost (Master Context §N).

### 2.5 Never

- Commit secrets (`secrets/`, `*.pem`, `*.key`, `broker.conf`, `wpa_supplicant.conf`, app `broker.json`), or run `git commit`/`git push` unless David asks.
- Log or publish a PIN, a password or a `cmd` payload.
- Rename classes, threads, events, topics, tables or files without approval.
- Modify an existing diagram without authorization.
- Implement from a document in `docs/archive/` or from any source listed as obsolete in Master Context §N.3.

### 2.6 Language

- Conversations with David: European Portuguese, with technical terms in English.
- Project documentation, slides, code, comments and commit messages: technical English.

### 2.7 Ownership

David: cloud, MQTT contract, `CommThread` and the `net`/`storage` classes, Flutter app. Ana: hardware, the `vigil_pwm` driver, the hardware-facing threads and `devices`/`hw` classes. Say so when a change crosses into the other person's area.

---

## 3. SOLID rules

### 3.1 Principle

SOLID is a course requirement and is applied pragmatically. An abstraction is introduced only when it has a concrete benefit:

- it allows real and simulated implementations (laptop simulation, tests);
- it isolates a C API or an external library;
- it is needed to test a class without hardware, clock or network;
- it removes a dependency between threads or packages.

Concurrency (threads, queues, priorities) is judged on timing; classes are judged on responsibilities: **merge threads, never responsibilities**.

### 3.2 SOLID Impact Analysis (required for every architectural proposal)

| Item | Content |
|---|---|
| Principles involved | S, O, L, I, D — only those that actually apply |
| Effect | Improved / preserved / violated, with the concrete reason |
| Better alternative | If one exists, and why it was not chosen |
| Trade-offs | Cost in classes, lines, effort, risk |
| Defensibility | How it would be explained to the professor |

Do not over-engineer to "show SOLID"; an abstraction with no benefit is a defect.

### 3.3 Code-review checklist (from the SOLID evidence map, DEC-009)

- **S:** a thread class only waits, calls components and sends its heartbeat; `AlarmLogic` holds every rule; `OutputPolicy` is the only owner of output rules; `Protocol` is the only place that builds or parses JSON; `EventStore`, `OutboxPublisher` and `MqttLink` keep their single jobs.
- **O:** a new sensor is a new `DebouncedInput` instance; a new tone is a new step sequence (no driver change); active levels and timings are configuration.
- **L:** every `WorkerThread` subclass follows the same life cycle, heartbeat and error policy; real and simulated hardware classes pass the same contract tests.
- **I:** interfaces stay small (`IGpioInput`, `IGpioOutput`, `IPwm`, `IPowerSensor`, `IMessageTransport`); no union-like command structs.
- **D:** `logic` includes no `hw`, `storage`, `net` or OS headers; time is injected; objects are created only in `main` (composition root).
- **Concurrency:** each domain object belongs to one thread; only the EventQueue and the CommQueue are shared; no dynamic allocation inside the real-time loops; no disk or network I/O in SCHED_FIFO threads.

### 3.4 Claims

Never claim in slides, report or code comments what the code does not do. In particular, never use "new event = new EventType value" as Open/Closed evidence (adding an event edits `AlarmLogic`). Where SOLID is deliberately not applied (Master Context §F.3) is documented but not presented.

---

## 4. Diagrams (draw.io)

1. Every **new or modified** technical diagram is made in **draw.io** (diagrams.net), unless David authorizes another tool.
2. Deliver three files: the editable `.drawio` source, an **SVG** export (documents) and a **PNG** export at 2× scale (slides).
3. Location and naming: `docs/diagrams/<area>/<name>.drawio` with `<name>.svg` and `<name>.png` beside it; areas: `architecture`, `hardware`, `software`, `fsm`, `flowcharts`, `driver`, `communication`, `database`, `tests`.
4. **Never modify an existing diagram without authorization.** Existing PlantUML or image diagrams stay as they are until a change is authorized; when one is changed, it is redrawn in draw.io.
5. Notation: UML 2.5 for class, package, state and sequence diagrams; standard flowchart shapes (terminator, process, decision, input/output) for flowcharts.
6. Every name in a diagram matches the Master Context and the code (classes, threads, events, states, topics).
7. One diagram, one purpose. Include a legend when colour or line style carries meaning; use colour for signal type or package, not decoration.
8. Readability at slide size: text no smaller than about 12 pt when the PNG fills the slide; split a diagram rather than shrink it.
9. Diagram text in English. Add a small footer with version and date.

---

## 5. Documentation maintenance (prevent drift)

### 5.1 Where things live

| File | Content | Changed when |
|---|---|---|
| `VIGILSYSTEM_MASTER_CONTEXT.md` | Current approved state + decision history | Any approved decision, status change or new evidence |
| `VIGILSYSTEM_GUIDELINES.md` | Working rules | A rule changes |
| `CLAUDE.md` | Entry point | The reading order or the document map changes |
| `docs/IMPLEMENTATION_GUIDE.md` | Detailed specification | The detail of a decided design is written or changes |
| `docs/hardware/HARDWARE_CONTEXT.md` | Hardware specification | Hardware decision or measurement |
| `docs/presentation/DESIGN_DECK_CHANGES.md` | Deck change plan | Presentation decisions |
| `docs/archive/` | Superseded documents | Only when a document is archived |

### 5.2 When a decision is completed

1. Claude writes the decision record (Appendix A) and a **documentation impact list**: every section and file that must change.
2. Claude asks for approval. Nothing is edited autonomously.
3. After approval: the current-state sections of the Master Context are **rewritten** (never appended with a second, contradicting description); the decision record goes to §G; superseded statements are marked with the decision that superseded them; the version, the date and the change log are updated.
4. The specialised documents in the impact list are updated in the same approved batch, or a discrepancy is recorded in Master Context §J.1 / §N.
5. The repository copy (canonical) and the Claude Project copy (`VigilSystem/…`) are synchronised; both show the same version.

### 5.3 Implementation progress

Master Context §J changes only with evidence (commit, test result, measurement). Never mark a component done because its design is finished. The status scale is: not started → in progress → implemented → integrated → tested → validated on hardware.

### 5.4 Consolidation checkpoint

At the end of a work block, before each presentation, or when David asks: check that the Master Context, the specialised documents and the code agree; list the discrepancies; propose the updates; record the date as "last consolidation".

### 5.5 Archiving

A superseded document is moved to `docs/archive/` with this banner at the top, and removed from the Claude Project only after David confirms:

> **ARCHIVED — superseded on <date> by `VIGILSYSTEM_MASTER_CONTEXT.md` v<x.y>. Kept for history only. Do not implement from this document.**

---

## 6. Technical rules

- C++17, namespace `vigil`, one class per header/source pair; class and package names as in Master Context §E.3.
- RAII for every file descriptor, line request, SQLite handle and mutex; no raw `new`/`delete`.
- Errors: expected device errors are reported as FAULT events (once per component); unexpected errors are caught in `WorkerThread::run()`, logged, and end in a watchdog reset; no exception crosses a thread boundary.
- `syslog()` with ident `vigil`; never log a PIN, a password or a `cmd` payload.
- SQLite: prepared statements only; times in ISO 8601 UTC text.
- Real time: SCHED_FIFO threads use absolute-time periodic sleeps (`CLOCK_MONOTONIC`); all blocking waits have a timeout; PI mutexes on shared queues.
- Every logic change comes with L1 tests; every hardware wrapper has a simulated implementation and passes the same contract tests.
- Build modes: `VIGIL_SIM=ON` (laptop), `VIGIL_TARGET=ON` (Pi, cross-compiled with the Buildroot toolchain).

---

## 7. Presentation rules

- Slides in English; the baseline for slide numbers is the 88-slide deck of 8 Nov 2026.
- Never present the items listed in Master Context §L.5 ("Never presented").
- Every SOLID note on a slide matches a line of the SOLID evidence map (Master Context §F.2).
- Divergences from the Analysis are not shown on the slides (DEC-013, P4-6); their reasons are in Master Context §B.8 and §G for oral questions.
- Claude edits slides only in batches David authorizes, and only the slides named in each batch (DEC-013, P4-9).
- Other groups' decks are references, not requirements; do not copy their text.

---

## Appendix A — Decision record template

```markdown
#### DEC-0xx — <title>

- **Problem.** <what is wrong or undecided, with evidence>
- **Current solution.** <what the documents or code say now>
- **Alternatives.** A — <…>; B — <…>; C — <…> (one of them: keep the current solution)
- **Analysis.** <effort, risk, requirements affected>
- **Recommendation.** <option and why>
- **SOLID Impact Analysis.** <table from §3.2, for architectural decisions>
- **Decision.** <option chosen by David, date> — Status: pending / approved / rejected / superseded by DEC-0yy
- **Consequences.** <what changes, accepted risks>
- **Affected.** <components, Master Context sections, documents, slides, tests>
- **Sources.** <conversation date, documents, files>
```
