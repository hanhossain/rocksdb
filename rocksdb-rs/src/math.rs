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

            pub fn [<count_ones_$id>](v: $id) -> i32 {
                v.count_ones() as i32
            }

            pub fn [<parity_$id>](v: $id) -> i32 {
                (v.count_ones() % 2) as i32
            }

            pub fn [<swap_bytes_$id>](v: $id) -> $id {
                v.swap_bytes()
            }

            pub fn [<reverse_bits_$id>](v: $id) -> $id {
                v.reverse_bits()
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
