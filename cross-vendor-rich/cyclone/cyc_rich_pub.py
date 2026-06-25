import sys, time
from cyclonedds.domain import DomainParticipant
from cyclonedds.pub import DataWriter, Publisher
from cyclonedds.topic import Topic
from cyclonedds.core import Qos, Policy
from cyc_rich import TYPES
dom=int(sys.argv[1]) if len(sys.argv)>1 else 0
ext=sys.argv[2] if len(sys.argv)>2 else "m"
n=int(sys.argv[3]) if len(sys.argv)>3 else 50
name,T=TYPES[ext]
dp=DomainParticipant(dom); t=Topic(dp,name,T)
w=DataWriter(Publisher(dp),t,qos=Qos(Policy.Reliability.Reliable(10000000000)))
print(f"[cyc-pub] dom={dom} ext={ext} topic={name} blob={n}",flush=True)
i=0
while True:
    i+=1; blob=[(k%251) for k in range(n)]
    w.write(T(id=i, value=3.141592653589793*i, blob=blob))
    print(f"  -> id={i} blob={n}",flush=True); time.sleep(0.15)
