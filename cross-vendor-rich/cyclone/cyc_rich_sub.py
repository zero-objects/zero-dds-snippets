import sys, time
from cyclonedds.domain import DomainParticipant
from cyclonedds.sub import DataReader, Subscriber
from cyclonedds.topic import Topic
from cyclonedds.core import Qos, Policy
from cyc_rich import TYPES
dom=int(sys.argv[1]) if len(sys.argv)>1 else 0
ext=sys.argv[2] if len(sys.argv)>2 else "m"
name,T=TYPES[ext]
dp=DomainParticipant(dom); t=Topic(dp,name,T)
r=DataReader(Subscriber(dp),t,qos=Qos(Policy.Reliability.BestEffort))
print(f"[cyc-sub] dom={dom} ext={ext} topic={name}",flush=True)
while True:
    for s in r.take(10):
        ok=all((b%256)==(k%251) for k,b in enumerate(s.blob))
        verdict='OK' if ok else 'CORRUPT'
        print(f"  <- id={s.id} value={s.value:.3f} blob={len(s.blob)} integrity={verdict}",flush=True)
    time.sleep(0.1)
