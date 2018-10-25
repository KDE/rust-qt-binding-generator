// Copyright 2017  Jos van den Oever <jos@vandenoever.info>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of
// the License or (at your option) version 3 or any later version
// accepted by the membership of KDE e.V. (or its successor approved
// by the membership of KDE e.V.), which shall act as a proxy
// defined in Section 14 of version 3 of the license.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

use interface::*;

#[derive(Default, Clone)]
struct TimeSeriesItem {
    time: f32,
    sin: f32,
    cos: f32,
}

pub struct TimeSeries {
    emit: TimeSeriesEmitter,
    list: Vec<TimeSeriesItem>,
}

impl TimeSeriesTrait for TimeSeries {
    fn new(emit: TimeSeriesEmitter, _: TimeSeriesList) -> TimeSeries {
        let mut series = TimeSeries {
            emit,
            list: Vec::new(),
        };
        for i in 0..101 {
            let x = i as f32 / 10.;
            series.list.push(TimeSeriesItem {
                time: x,
                sin: x.sin(),
                cos: x.cos(),
            });
        }
        series
    }
    fn emit(&mut self) -> &mut TimeSeriesEmitter {
        &mut self.emit
    }
    fn row_count(&self) -> usize {
        self.list.len() as usize
    }
    fn time(&self, row: usize) -> f32 {
        self.list[row as usize].time
    }
    fn set_time(&mut self, row: usize, v: f32) -> bool {
        self.list[row as usize].time = v;
        true
    }
    fn sin(&self, row: usize) -> f32 {
        self.list[row as usize].sin
    }
    fn set_sin(&mut self, row: usize, v: f32) -> bool {
        self.list[row as usize].sin = v;
        true
    }
    fn cos(&self, row: usize) -> f32 {
        self.list[row as usize].cos
    }
    fn set_cos(&mut self, row: usize, v: f32) -> bool {
        self.list[row as usize].cos = v;
        true
    }
}
