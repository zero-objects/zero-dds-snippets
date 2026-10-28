// Application code against the generated DLRL interfaces: the keys carry
// the IDL's types, so a venue is a string and an id a number; `price` is a
// mono-valued attribute the application sets, `tags` and `fills` are lists
// and `fees` a map, each changed through the collection itself.
import { Leg, LegImpl, Trade, TradeImpl } from "./gen/trade_dlrl";

const leg: Leg = new LegImpl();
const trade: Trade = new TradeImpl();
trade.price = 101.25;
trade.tags.push("fx", "spot");
trade.fills.push(40, 60);
trade.fees.set("clearing", 0.25);
const venue: string = trade.venue;
const id: number = trade.id + leg.id;
const price: number = trade.price;
const legs: Leg[] = trade.legs;
const tags: string[] = trade.tags;
const filled: number = trade.fills.reduce((a, b) => a + b, 0);
const clearing: number | undefined = trade.fees.get("clearing");
console.log(`DLRL TYPESCRIPT OK: venue="${venue}" id=${id} price=${price} legs=${legs.length} tags=${tags.join(",")} filled=${filled} clearing=${clearing}`);
