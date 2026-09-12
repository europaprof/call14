# Call14 Elevator Floor Recognizer v6.0.1

This patch keeps the v6 alignment architecture and fixes two production cases
found during live observation after the initial release.

At floor 14, the red and geometry decoders could simultaneously lose the leading
`1` because they shared the same tens mask. Their apparent agreement was not
independent and could overwrite 14 with 4 while the elevator was idle. Recovery
now requires a fresh whole-digit template plus separate white-channel evidence
for the tens digit. Cold startup uses the same protection.

Floors 8 and 9 differ only by the lower-left segment. When the red and geometry
decoders disagree exactly on that pair, v6.0.1 holds the confirmed floor unless
a focused whole-digit comparison corroborates the physical geometry.

The final public build passed 361/361 labelled observations on the primary ride,
236/236 on an independent holdout ride, targeted 14/4 recovery, and exact 8/9
conflict tests in both directions.

Camera URLs, credentials, private addresses and recordings are not included.
