# Coolgreen Aux repo 🌀

This repository  contains three AC IR control projects 🌬️

## IR Learner 📚

The IR Learner is a C program designed for Linux, enabling you to capture and learn IR AC commands. It generates MySQL INSERT statements for the learned commands, providing the foundation for integrated IR remote control functionality. 📝

### Components 🛠️

1. **IR Learner Program:** The core C program responsible for capturing and learning IR commands.

2. **Query Reductor:** A tool that optimizes MySQL queries. It identifies constant and variable bits within the AC commands and generates new MySQL queries containing only the variable segments. The constant bits are stored as part of the tag, simplifying command interpretation. 🧩

## IR Sniffer (Arduino) 🕵️‍♂️

The IR Sniffer, an Arduino-based component, serves as the IR command decoder. It captures IR commands, translates them into pulse distance modulation data, and displays this data on the console. This component is indispensable for understanding the structure of IR commands and plays a vital role in the learning process. 🔍

## Coprocessor Debugger 🧰

The Coprocessor Debugger folder houses a software tool that emulates the master board of the gateway (ESP). This emulator is a resource for debugging the coprocessor's (NRF) program. By using a development kit (DK), we can develop and debug our code. 🚀

---
