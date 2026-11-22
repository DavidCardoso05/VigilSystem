# Archive — superseded documents

> **Nothing in this folder is current. Do not implement from these documents.**
> The single source of truth is `VIGILSYSTEM_MASTER_CONTEXT.md` (v1.0, 2026-11-09). These files are kept only to show how the design evolved.

Archived on 2026-11-09 (Master Context v1.0 consolidation). Each file starts with an "ARCHIVED" box; the text after the box is an unchanged copy of the original Claude Project document.

| File | Original Claude Project document | Date | Content | Superseded by |
|---|---|---|---|---|
| [`2026-11-09_Simplification_Decision_Log_PT.md`](2026-11-09_Simplification_Decision_Log_PT.md) | `claude/VigilSystem – Simplification Decision Log.md` | 8–9 Nov 2026 | Portuguese record of the simplification review, DEC-001…DEC-010 | Master Context §G (DEC-001…DEC-012), §H |
| [`2026-11-09_Presentation_Changes_PT.md`](2026-11-09_Presentation_Changes_PT.md) | `claude/VigilSystem – Presentation Changes (DEC-001 to DEC-006).md` | 9 Nov 2026 | Portuguese slide-by-slide change list for the 88-slide deck (DEC-001…DEC-010) | `docs/presentation/DESIGN_DECK_CHANGES.md` v1.0 |
| [`2026-11-04_Implementation_Guide_Project_copy.md`](2026-11-04_Implementation_Guide_Project_copy.md) | `claude/VigilSystem – Implementation Guide.md` | 4 Nov 2026 | Project copy of the Implementation Guide; differs from the repository copy in the RGB lines of §4.3 and §12.2 | `docs/IMPLEMENTATION_GUIDE.md` (repository copy with validity box) and Master Context |
| [`2026-11-04_Design_Phase_Revision_Plan.md`](2026-11-04_Design_Phase_Revision_Plan.md) | `VigilSystem — Design Phase Revision Plan.md` | 4 Nov 2026 | Slide-by-slide review of the 71-slide deck; contradictions C1–C22; open decisions D1–D16 | Master Context §G.3, §L; `docs/presentation/DESIGN_DECK_CHANGES.md` |
| [`2026-11-04_Design_Phase_Revision_Plan_pointer.md`](2026-11-04_Design_Phase_Revision_Plan_pointer.md) | `claude/VigilSystem – Design Phase Revision Plan (pointer).md` | 4 Nov 2026 | Short summary of the Revision Plan with a link to its artifact | Master Context §G.3, §L |
| [`2026-11-04_Class_Diagrams_for_Slides_PlantUML.md`](2026-11-04_Class_Diagrams_for_Slides_PlantUML.md) | `claude/VigilSystem – Class Diagrams for Slides (PlantUML).md` | 4 Nov 2026 | PlantUML sources of three class diagrams for the 9-thread architecture | Master Context §E.3 (diagrams D12–D14 to be redrawn in draw.io) |
| [`2026-11-02_Design_Decisions_Record.md`](2026-11-02_Design_Decisions_Record.md) | `VigilSystem – Design Decisions Record.md` | 2 Nov 2026 | Design decisions of the 61-slide deck review | Master Context §G.3 (D-01…D-51), §E, §I |
| [`2026-11-02_Class_Diagram_Design_Phase.md`](2026-11-02_Class_Diagram_Design_Phase.md) | `VigilSystem – Class Diagram (Design Phase).md` | 2 Nov 2026 | Class diagram v2 for the 9-thread architecture (PlantUML) | Master Context §E.3 |
| [`2026-11-02_Data_Storage_ERD_Cloud_Updates.md`](2026-11-02_Data_Storage_ERD_Cloud_Updates.md) | `VigilSystem – Design Deck Data Storage, ERD and Cloud Architecture Updates.md` | 2 Nov 2026 | Data storage, 3-table ERD and cloud architecture slide updates | Master Context §E.5, §I |
| [`2026-11-02_Package_Diagram_Design_Phase.md`](2026-11-02_Package_Diagram_Design_Phase.md) | `VigilSystem – Package Diagram (Design Phase).md` | 2 Nov 2026 | Package diagram for the 9-thread architecture (PlantUML) | Master Context §E.3 |
| [`2026-11-02_Diagrams_Work_Log_and_Rationale.md`](2026-11-02_Diagrams_Work_Log_and_Rationale.md) | `VigilSystem – Design-Phase Diagrams Work Log and Rationale.md` | 2 Nov 2026 | FSM and flowchart work log; open issues 1–28 | Master Context §E.4, §G |
| [`2026-10-31_Mobile_App_GUI_Design.md`](2026-10-31_Mobile_App_GUI_Design.md) | `VigilSystem_Mobile_App_GUI_Design.md` | 31 Oct 2026 | GUI decisions, Figma Make prototype, Flutter plan | Master Context §I.7 |
| [`2026-10-31_Hardware_Context_v1.md`](2026-10-31_Hardware_Context_v1.md) | `VigilSystem_Hardware_Context.md` | 31 Oct 2026 | Hardware decisions of the design-phase review (version 1) | `docs/hardware/HARDWARE_CONTEXT.md` v2.0 |
| [`2026-11-03_G8_VigilSystem_D_Presentation_61_slides.pdf`](2026-11-03_G8_VigilSystem_D_Presentation_61_slides.pdf) | Repository `docs/design/G8_VigilSystem_D_Presentation.pdf` (archived 2026-11-17; PDF, so no ARCHIVED box) | 3 Nov 2026 | PDF export of the 61-slide design deck | `docs/design/G8_VigilSystem_D_Presentation.pdf` (82 slides, 17 Nov) |
| [`2026-10-25_MQTT_Broker_Selection.md`](2026-10-25_MQTT_Broker_Selection.md) | `VigilSystem – MQTT Broker Selection EMQX Serverless and Mosquitto Test Broker.md` | 25 Oct 2026 | Broker comparison and selection of EMQX Serverless; Mosquitto as test broker | Master Context §I.1–§I.2 (the broker choice itself is still valid) |

Not copied here:

- The old `CLAUDE.md` (4 Nov) is in the git history (commit of 4 Nov) and in `~/VigilSystem-old-claude-outputs/`, outside the repository (moved there from `Claude outputs/` on 2026-11-17).
- The repository copy of the Implementation Guide stays at `docs/IMPLEMENTATION_GUIDE.md` with a validity box, because some of its sections are still valid.
- The decks (`.pptx`) are not archived here; the current design deck PDF stays in `docs/design/`.

Whether the original documents are also removed from the Claude Project is a separate decision for David.
