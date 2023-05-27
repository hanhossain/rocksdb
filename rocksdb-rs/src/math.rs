use paste::paste;

macro_rules! gen_math {
    ($id:ident) => {
        paste! {
            pub fn [<floor_log2_$id>](v: $id) -> i32 {
                v.ilog2() as i32
            }

            pub fn [<trailing_zeros_$id>](v: $id) -> i32 {
                v.trailing_zeros() as i32
            }
        }
    };
}

gen_math!(i8);
gen_math!(i16);
gen_math!(i32);
gen_math!(i64);
gen_math!(u8);
gen_math!(u16);
gen_math!(u32);
gen_math!(u64);
