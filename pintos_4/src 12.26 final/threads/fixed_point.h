/*use fixed-point arithmetic operations to realize*/ 

/*check if fixed-point heve been defined*/
#ifndef fixed_t

/*define the type*/
#define fixed_t int

/*14 bits used for fractional part*/
#define fixed_shift_bit 14

/*Convert n to fixed point*/
#define fixed_value(x) ((fixed_t)(x << fixed_shift_bit))

/*Convert x to integer (rounding toward zero)*/
#define fixed_integer(x) (x >> fixed_shift_bit)

/*Convert x to integer (rounding to nearest)*/
#define fixed_integer_round(x) (x >= 0 ? \
      ((x + (1 << (fixed_shift_bit - 1))) >> fixed_shift_bit): \
	  ((x - (1 << (fixed_shift_bit - 1))) >> fixed_shift_bit))

/* Add two fixed-point value */
#define fixed_add(x,y) (x + y)

/* Add a fixed-point value and an int value*/
#define fixed_add_to_left(x,n)  (x + (n << fixed_shift_bit))

/* Subtract two fixed-point value. */
#define fixed_sub(x,y)  (x - y)

/* Subtract an int value  from a fixed-point value*/
#define fixed_sub_from_left(x,y) (x - (y << fixed_shift_bit))

/* Multiply a fixed-point value by an int value */
#define fixed_mult_to_left(x,n)  (x * n)

/* Multiply two fixed-point values */
#define fixed_mult(x,y) ((fixed_t)(((int64_t) x) * y >>  fixed_shift_bit))

/* Divide a fixed-point value by an int value */
#define fixed_div_from_left(x,n) (x / n)

/* Divide two fixed-point values*/
#define fixed_div(x,y) ((fixed_t)((((int64_t) x) << fixed_shift_bit) / y))


#endif


