import nextpnrpy_ice40
net = ctx.nets["longwire"]

cursor = ctx.getBelPinWire(net.driver.cell.bel, net.driver.port)
sink = ctx.getBelPinWire(net.users[0].cell.bel, net.users[0].port)

ctx.bindWire(cursor, net, nextpnrpy_ice40.STRENGTH_LOCKED)

target_delay = 2000
accum_delay = 0
# First, follow highest delay until we reach target
visited = set()
last_pip = None
while True:
    next = None
    next_delay = 0
    next_ult_delay = 99999
    print("   Visiting ", cursor)
    for downhill in ctx.getPipsDownhill(cursor):
        if downhill in visited:
            continue
        if not ctx.checkPipAvail(downhill):
            continue
        dest = ctx.getPipDstWire(downhill)
        if not ctx.checkWireAvail(dest):
            continue
        if accum_delay > target_delay:
            if ctx.estimateDelay(dest, sink) < next_ult_delay:
                next_ult_delay = ctx.estimateDelay(dest, sink)
                next = downhill
        else:
            if ctx.getPipDelay(downhill).maxDelay() > next_delay:
                next_delay = ctx.getPipDelay(downhill).maxDelay()
                next = downhill
    if next is None:
        ctx.unbindPip(last_pip)
        cursor = ctx.getPipSrcWire(last_pip)
        accum_delay -= ctx.getDelayNS(ctx.getPipDelay(last_pip).maxDelay())
        last_pip = net.wires[cursor].pip
        continue
    visited.add(next)
    ctx.bindPip(next, net, nextpnrpy_ice40.STRENGTH_LOCKED)
    last_pip = next
    cursor = ctx.getPipDstWire(next)
    accum_delay += ctx.getDelayNS(ctx.getPipDelay(next).maxDelay())
    print(downhill, accum_delay)
    if accum_delay > target_delay and cursor == sink:
        break