module top(input btn, output d0, output d1);

assign d0 = btn;

(* keep *) SB_LUT4 #(.LUT_INIT(2)) buf0_i(.I0(btn), .I1(), .I2(), .I3(), .O(longwire));

(* keep *) SB_LUT4 #(.LUT_INIT(2)) buf1_i(.I0(longwire), .I1(), .I2(), .I3(), .O(d1));

endmodule
