// The necessary tests:
There are 3 tests present:

1. A simple test to send and receive message, the files are recv_reply.c  snd_recv.c.
2. The max size test. The maximum size of a message depends on a number of factors:
   MTU size (set through kcp_setmtu, default window receive size and the send window size,
   please see the README in parent directory.
   The files for this test are max_size_reply.c  max_size_snd.c.
   It works as follows: the messages of increasing size are sent from one peer to another
   and then being echoed back. As soon as this breaks, we have reached the max size. We increment
   the message size by bit shifting, so the size increments grow larger. Feel free to use different
   technique for finer measurements.
3. Multiple sockets test (mt_reply.c  mt_snd.c). The purpose of this test is to simulate multiple wmls opening and closing. This test works as follows:
    a. A range of sockets is established.
    b. One peer creates listening sockets for the entire range.
    c. Another peer for every socket in parallel in the range does the following (in order):
      - randomly sleeps.
      - creates a wml.
      - establishes the character - this is calculated based on an offset from the start of the range
      - sends  messages of the sizes: 10K, 100K, 500K, 2MB with random sleeping in-between, but no more than 3 secs.
      - The listening peer echoes the messages back and sending peer verifies that it is the original message.
      - Once all the messages have been accounted for the sending peer closes the connection.
      
    

