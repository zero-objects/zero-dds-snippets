// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! A date-time group, `DDHHMMZMONYY`: a type that cannot hold an invalid
//! one, standing for an IDL `string`.

use core::fmt;
use core::str::FromStr;

use zerodds_wire::{IdlOut, WireError, WireRepr, Written};

const MONTHS: [&str; 12] = [
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
];

/// A date-time group: day, hour and minute in UTC, month, two-digit year.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct Dtg {
    year: u8,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
}

/// Why a text is no date-time group.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct NotADtg;

impl fmt::Display for NotADtg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("a date-time group DDHHMMZMONYY")
    }
}

impl std::error::Error for NotADtg {}

impl FromStr for Dtg {
    type Err = NotADtg;

    fn from_str(text: &str) -> Result<Self, NotADtg> {
        let b = text.as_bytes();
        if b.len() != 12 || b[6] != b'Z' {
            return Err(NotADtg);
        }
        let two = |i: usize| -> Result<u8, NotADtg> {
            let (tens, units) = (b[i], b[i + 1]);
            if tens.is_ascii_digit() && units.is_ascii_digit() {
                Ok((tens - b'0') * 10 + (units - b'0'))
            } else {
                Err(NotADtg)
            }
        };
        let month = MONTHS
            .iter()
            .position(|month| month.as_bytes() == &b[7..10])
            .ok_or(NotADtg)?;
        let dtg = Self {
            day: two(0)?,
            hour: two(2)?,
            minute: two(4)?,
            month: u8::try_from(month + 1).map_err(|_| NotADtg)?,
            year: two(10)?,
        };
        if (1..=31).contains(&dtg.day) && dtg.hour < 24 && dtg.minute < 60 {
            Ok(dtg)
        } else {
            Err(NotADtg)
        }
    }
}

impl fmt::Display for Dtg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{:02}{:02}{:02}Z{}{:02}",
            self.day,
            self.hour,
            self.minute,
            MONTHS[usize::from(self.month - 1)],
            self.year
        )
    }
}

/// An IDL `string` on the wire: written with `Display` straight into the
/// sample's buffer, read from the `&str` the received sample lends.
impl WireRepr for Dtg {
    type Idl = str;

    fn to_wire(&self, out: IdlOut<'_, str>) -> Result<Written, WireError> {
        out.display(self)
    }

    fn from_wire(wire: &str) -> Result<Self, WireError> {
        wire.parse()
            .map_err(|error: NotADtg| WireError::invalid_value(error.to_string()))
    }
}
