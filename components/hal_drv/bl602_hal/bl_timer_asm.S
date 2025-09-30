# Export functions globally
.global timer_us_now, bl_timer_delay_us

# Required memory addresses
.set TICK_HIGH,    0x0200BFFC
.set TICK_LOW,     0x0200BFF8

# Code
.text

# Gets current timer value
timer_us_now:
1:
    # Store high tick value
    la   t0, TICK_HIGH # Load address
    lw   t1, 0(t0)     # Store contents of address

    # Store low tick value
    la   t0, TICK_LOW  # Load address
    lw   a0, 0(t0)     # Store contents of address -> directly in return value so no copy operation required if we are done

    # Store high tick value again
    la   t0, TICK_HIGH # Load address
    lw   a1, 0(t0)     # Store contents of address -> directly in return value so no copy operation required if we are done
    
    # Loop this procedure until values in t1 and a1 are different so that we know a tick passed
    bne  t1, a1, 1b

    # Return if values are equal
    ret

# Delays by amount of us handed over in a0
bl_timer_delay_us:
    # Store current low tick value in t1
    la   t0, TICK_LOW # Load address
    lw   t1, 0(t0)    # Store contents of address

    # Calculate ticks required in t2 (us to ticks, each us are 10 timer ticks)
    li   t0, 10      # Load multiplier
    mul  t2, t0, a0  # t2 = t0 * a0 = us * 10

    # Increse current low tick value by required ticks to know when the time is over
    add  t0, t1, t2  # t0 = t1 + t2 = Current ticks + required ticks. t0 contains the deadline now
1:
    # Read current low tick value to t1
    la   t2, TICK_LOW # Load address
    lw   t1, 0(t2)    # Store contents of address

    # Repeat while current tick value is smaller than the deadline
    bltu t1, t0, 1b

    # We are done if the current tick value is equal or larger the deadline
    ret
