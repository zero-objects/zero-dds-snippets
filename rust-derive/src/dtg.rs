// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 ZeroDDS Contributors
//! A date-time group, `DDHHMMZMONYY`: day, hour, minute, `Z` for UTC,
//! month, year. A value of this type is always a valid one.

use core::fmt;

use zerodds_codec::WireRepr;

const MONTHS: [&str; 12] = [
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
];

/// `derive(WireRepr)`: an IDL `string` on the wire, read through
/// `TryFrom<&str>` from the text the received sample lends, written through
/// `Display` into the sample's buffer.
#[derive(WireRepr, Debug, Clone, Copy, PartialEq, Eq)]
#[wire(idl = "string")]
pub struct Dtg {
    day: u8,
    hour: u8,
    minute: u8,
    month: u8,
    year: u8,
}

/// Why a text is no date-time group.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct NotADtg;

impl fmt::Display for NotADtg {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("a date-time group DDHHMMZMONYY")
    }
}

impl std::error::Error for NotADtg {}

impl TryFrom<&str> for Dtg {
    type Error = NotADtg;

    fn try_from(text: &str) -> Result<Self, Self::Error> {
        let bytes = text.as_bytes();
        if bytes.len() != 12 || bytes[6] != b'Z' {
            return Err(NotADtg);
        }
        let two = |at: usize| -> Result<u8, NotADtg> {
            let (tens, units) = (bytes[at], bytes[at + 1]);
            if tens.is_ascii_digit() && units.is_ascii_digit() {
                Ok((tens - b'0') * 10 + (units - b'0'))
            } else {
                Err(NotADtg)
            }
        };
        let month = MONTHS
            .iter()
            .position(|name| name.as_bytes() == &bytes[7..10])
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

impl core::str::FromStr for Dtg {
    type Err = NotADtg;

    fn from_str(text: &str) -> Result<Self, Self::Err> {
        Self::try_from(text)
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
