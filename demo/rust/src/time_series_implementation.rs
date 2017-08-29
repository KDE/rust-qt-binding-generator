use time_series_interface::*;

#[derive(Default, Clone)]
struct TimeSeriesItem {
    input: u32,
    result: u32,
}

pub struct TimeSeries {
    emit: TimeSeriesEmitter,
    model: TimeSeriesList,
    list: Vec<TimeSeriesItem>,
}

impl TimeSeriesTrait for TimeSeries {
    fn create(emit: TimeSeriesEmitter, model: TimeSeriesList) -> TimeSeries {
        let mut series = TimeSeries {
            emit: emit,
            model: model,
            list: Vec::new(),
        };
        for i in 0..100 {
            series.list.push(TimeSeriesItem {
                input: i,
                result: 2 * i,
            });
        }
        series
    }
    fn emit(&self) -> &TimeSeriesEmitter {
        &self.emit
    }
    fn row_count(&self) -> usize {
        self.list.len() as usize
    }
    fn input(&self, row: usize) -> u32 {
        self.list[row as usize].input
    }
    fn set_input(&mut self, row: usize, v: u32) -> bool {
        self.list[row as usize].input = v;
        true
    }
    fn result(&self, row: usize) -> u32 {
        self.list[row as usize].result
    }
    fn set_result(&mut self, row: usize, v: u32) -> bool {
        self.list[row as usize].result = v;
        true
    }
}
