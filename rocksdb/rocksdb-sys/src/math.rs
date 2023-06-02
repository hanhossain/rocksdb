use paste::paste;
use std::mem::size_of;

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

            /// Every output bit depends on many input bits in the same and higher
            /// positions, but not lower positions. Specifically, this function
            /// * Output highest bit set to 1 is same as input (same FloorLog2, or
            ///   equivalently, same number of leading zeros)
            /// * Is its own inverse (an involution)
            /// * Guarantees that b bottom bits of v and c bottom bits of
            ///   DownwardInvolution(v) uniquely identify b + c bottom bits of v
            ///   (which is all of v if v < 2**(b + c)).
            /// ** A notable special case is that modifying c adjacent bits at
            ///    some chosen position in the input is bijective with the bottom c
            ///    output bits.
            /// * Distributes over xor, as in DI(a ^ b) == DI(a) ^ DI(b)
            ///
            /// This transformation is equivalent to a matrix*vector multiplication in
            /// GF(2) where the matrix is recursively defined by the pattern matrix
            /// P = | 1 1 |
            ///     | 0 1 |
            /// and replacing 1's with P and 0's with 2x2 zero matices to some depth,
            /// e.g. depth of 6 for 64-bit T. An essential feature of this matrix
            /// is that all square sub-matrices that include the top row are invertible.
            pub fn [<downward_involution_$id>](v: $id) -> $id {
                let mut r = v as u64;

                if size_of::<$id>() > 4 {
                    r ^= r >> 32;
                }

                if size_of::<$id>() > 2 {
                    r ^= (r & 0xffff0000ffff0000) >> 16;
                }

                if size_of::<$id>() > 1 {
                    r ^= (r & 0xff00ff00ff00ff00) >> 8;
                }

                r ^= (r & 0xf0f0f0f0f0f0f0f0) >> 4;
                r ^= (r & 0xcccccccccccccccc) >> 2;
                r ^= (r & 0xaaaaaaaaaaaaaaaa) >> 1;
                r as $id
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_downward_involution_i8() {
        let result = downward_involution_i8(i8::MAX);
        assert_eq!(downward_involution_i8(result), i8::MAX);

        let result = downward_involution_i8(i8::MIN);
        assert_eq!(downward_involution_i8(result), i8::MIN);
    }

    #[test]
    fn test_downward_involution_i16() {
        let result = downward_involution_i16(i16::MAX);
        assert_eq!(downward_involution_i16(result), i16::MAX);

        let result = downward_involution_i16(i16::MIN);
        assert_eq!(downward_involution_i16(result), i16::MIN);
    }

    #[test]
    fn test_downward_involution_i32() {
        let result = downward_involution_i32(i32::MAX);
        assert_eq!(downward_involution_i32(result), i32::MAX);

        let result = downward_involution_i32(i32::MIN);
        assert_eq!(downward_involution_i32(result), i32::MIN);
    }

    #[test]
    fn test_downward_involution_i64() {
        let result = downward_involution_i64(i64::MAX);
        assert_eq!(downward_involution_i64(result), i64::MAX);

        let result = downward_involution_i64(i64::MIN);
        assert_eq!(downward_involution_i64(result), i64::MIN);
    }

    #[test]
    fn test_downward_involution_u8() {
        let result = downward_involution_u8(u8::MAX);
        assert_eq!(downward_involution_u8(result), u8::MAX);

        let result = downward_involution_u8(u8::MIN);
        assert_eq!(downward_involution_u8(result), u8::MIN);
    }

    #[test]
    fn test_downward_involution_u16() {
        let result = downward_involution_u16(u16::MAX);
        assert_eq!(downward_involution_u16(result), u16::MAX);

        let result = downward_involution_u16(u16::MIN);
        assert_eq!(downward_involution_u16(result), u16::MIN);
    }

    #[test]
    fn test_downward_involution_u32() {
        let result = downward_involution_u32(u32::MAX);
        assert_eq!(downward_involution_u32(result), u32::MAX);

        let result = downward_involution_u32(u32::MIN);
        assert_eq!(downward_involution_u32(result), u32::MIN);
    }

    #[test]
    fn test_downward_involution_u64() {
        let result = downward_involution_u64(u64::MAX);
        assert_eq!(downward_involution_u64(result), u64::MAX);

        let result = downward_involution_u64(u64::MIN);
        assert_eq!(downward_involution_u64(result), u64::MIN);
    }
}
