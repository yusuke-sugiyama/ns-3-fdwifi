#!/bin/sh

CMD="select exp.input, 12 / (avg(sig.value) / 1000000000) \
    from Experiments exp, Singletons sig\
    where exp.run = sig.run\
    and sig.variable = 'endTime-total'\
    group by exp.input\
    order by abs(exp.input) ASC;"

sqlite3 -noheader data.db "$CMD" > throughput.data
sed -i.bak "s/|/   /" throughput.data
rm throughput.data.bak
echo "Done; throughput in throughput.data"


CMD="select exp.input, avg(sig2.value) / avg(sig1.value) \
    from Singletons sig1, Singletons sig2, Experiments exp\
    where sig1.run = exp.run  AND\
          sig2.run = sig1.run AND\
          sig1.variable='phy-tx-primary' AND\
          sig2.variable='phy-tx-secondary'\
    group by exp.input\
    order by abs(exp.input) ASC;"

sqlite3 -noheader data.db "$CMD" > fullduplex.data
sed -i.bak "s/|/   /" fullduplex.data
rm fullduplex.data.bak
echo "Done; full duplex ratio in fullduplex.data"


CMD="select exp.input, avg(sig3.value) / (avg(sig1.value) + avg(sig2.value)) \
    from Singletons sig1, Singletons sig2, Singletons sig3, Experiments exp\
    where sig1.run = exp.run AND\
          sig2.run = sig1.run AND\
          sig3.run = sig2.run AND\
          sig1.variable='phy-tx-primary'  AND\
          sig2.variable='phy-tx-secondary'AND\
          sig3.variable='mac-total-missed-ack'\
    group by exp.input\
    order by abs(exp.input) ASC;"

sqlite3 -noheader data.db "$CMD" > collision.data
sed -i.bak "s/|/   /" collision.data
rm collision.data.bak
echo "Done; collision ratio in collision.data"


CMD="select exp.input, avg(sig.value) \
    from Singletons sig, Experiments exp\
    where sig.run = exp.run AND\
          sig.variable='delay-average'
    group by exp.input\
    order by abs(exp.input) ASC;"

sqlite3 -noheader data.db "$CMD" > delay.data
sed -i.bak "s/|/   /" delay.data
rm delay.data.bak
echo "Done; delay in delay.data"
