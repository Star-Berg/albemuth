# DAC reference gate change

This revision changes only the physical-output gating behavior:

- Output off: Vsw low, DACA=0, DACB=0.
- Output requested: DACA/DACB output their set references while Vsw remains low during the existing 20 ms delay.
- Delay completed: Vsw goes high; DAC references remain active.
- Manual off or latched fault: Vsw is forced low and both DACs are cleared in the same control callback.
- Startup: Vsw, DACA and DACB are explicitly initialized to zero.

No CV/CC, protection, keypad, encoder, measurement, display or alarm logic was changed.

Host-side state tests passed for OFF, precharge delay, ON and FAULT states. TI CCS/C2000 target compilation and board testing are still required.
