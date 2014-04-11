NEUcs3600project4
=================

Simple Transport Protocol project for Systems &amp; Networks (Northeastern University)

Sliding window concept (Sender):
- Assumed window size of 5 packets. Can (should) be changed.
- Keep buffer of packets that need to be sent. Buffer should be the same as window size.
- Read packets until buffer is full.
- Send out one at a time, up to a maximum of five packets.
- Once an ack is received for a packet:
  - Remove the ack'd packet from the buffer
  - Retrieve the next packet from stdout
  - Send the next packet in line to be sent

Proposal: On ack timeout:
- Should a sent out packet's ack timeout in x seconds:
  - Retransmit all packets sent after the timed out packet.
  - Receiver should be able to deal with duplicate packets.
  - Eventually consider sending only the timed out packet to save bandwidth.