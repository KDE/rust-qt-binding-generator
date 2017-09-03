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
    fn create(emit: TimeSeriesEmitter, _: TimeSeriesList) -> TimeSeries {
        let mut series = TimeSeries {
            emit: emit,
            list: Vec::new(),
        };
        for i in 0..100 {
            let x = i as f32 / 10.;
            series.list.push(TimeSeriesItem {
                time: x,
                sin: x.sin(),
                cos: x.cos(),
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
