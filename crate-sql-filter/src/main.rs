// The website's docs/sql-filter.html usage snippet, made runnable.
// DDS 1.4 Annex B: a content filter expression with one positional parameter.
use std::collections::HashMap;
use zerodds_sql_filter::{parse, RowAccess, Value};

struct MapRow(HashMap<String, Value>);

impl RowAccess for MapRow {
    fn get(&self, path: &str) -> Option<Value> {
        self.0.get(path).cloned()
    }
}

fn main() {
    let expr = parse("color = %0 AND x > 10").unwrap();
    let row = MapRow(HashMap::from([
        ("color".into(), Value::String("RED".into())),
        ("x".into(), Value::Int(42)),
    ]));
    let params = [Value::String("RED".into())];
    assert_eq!(expr.evaluate(&row, &params), Ok(true));
    println!("sql-filter: \"color = %0 AND x > 10\" evaluated to true");
}
