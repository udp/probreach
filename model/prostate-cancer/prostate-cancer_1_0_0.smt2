(set-logic QF_NRA_ODE)
(declare-fun alphay () Real)
(declare-fun alphay_0_0 () Real)
(declare-fun alphay_0_t () Real)
(declare-fun alphay_1_0 () Real)
(declare-fun alphay_1_t () Real)
(declare-fun tau () Real)
(declare-fun tau_0_0 () Real)
(declare-fun tau_0_t () Real)
(assert (>= tau_0_0  0))
(assert (>= tau_0_t  0))
(assert (<= tau_0_0  100.0))
(assert (<= tau_0_t  100.0))
(declare-fun tau_1_0 () Real)
(declare-fun tau_1_t () Real)
(assert (>= tau_1_0  0))
(assert (>= tau_1_t  0))
(assert (<= tau_1_0  100.0))
(assert (<= tau_1_t  100.0))
(declare-fun x () Real)
(declare-fun x_0_0 () Real)
(declare-fun x_0_t () Real)
(assert (>= x_0_0  0))
(assert (>= x_0_t  0))
(assert (<= x_0_0  100.0))
(assert (<= x_0_t  100.0))
(declare-fun x_1_0 () Real)
(declare-fun x_1_t () Real)
(assert (>= x_1_0  0))
(assert (>= x_1_t  0))
(assert (<= x_1_0  100.0))
(assert (<= x_1_t  100.0))
(declare-fun y () Real)
(declare-fun y_0_0 () Real)
(declare-fun y_0_t () Real)
(assert (>= y_0_0  0))
(assert (>= y_0_t  0))
(assert (<= y_0_0  10.0))
(assert (<= y_0_t  10.0))
(declare-fun y_1_0 () Real)
(declare-fun y_1_t () Real)
(assert (>= y_1_0  0))
(assert (>= y_1_t  0))
(assert (<= y_1_0  10.0))
(assert (<= y_1_t  10.0))
(declare-fun z () Real)
(declare-fun z_0_0 () Real)
(declare-fun z_0_t () Real)
(assert (>= z_0_0  0.0))
(assert (>= z_0_t  0.0))
(assert (<= z_0_0  100.0))
(assert (<= z_0_t  100.0))
(declare-fun z_1_0 () Real)
(declare-fun z_1_t () Real)
(assert (>= z_1_0  0.0))
(assert (>= z_1_t  0.0))
(assert (<= z_1_0  100.0))
(assert (<= z_1_t  100.0))
(declare-fun time_0 () Real)
(assert (>= time_0  0))
(assert (<= time_0  100.0))
(declare-fun time_1 () Real)
(assert (>= time_1  0))
(assert (<= time_1  100.0))
(define-ode flow_1 ((= d/dt[alphay]  0)(= d/dt[tau] (* 1.0 1.0))(= d/dt[x] (* 1.0(+(*(-(-(-(/ 0.0197(+ 1(exp(*(- 10.0 z) 1.0))))(/ 0.0175(+ 1(exp(*(- z 10.0) 2)))))(* 0.00005(- 1(/ z 12.0)))) 0.01) x) 0.03)))(= d/dt[y] (* 1.0(+(*(* 0.00005(- 1(/ z 12.0))) x)(*(-(* alphay(- 1(* 1.0(/ z 12.0)))) 0.0168) y))))(= d/dt[z] (* 1.0(+(*(- z) 0.08) 0.02)))))
(define-ode flow_2 ((= d/dt[alphay]  0)(= d/dt[tau] (* 1.0 1.0))(= d/dt[x] (* 1.0(+(*(-(-(-(/ 0.0197(+ 1(exp(*(- 10.0 z) 1.0))))(/ 0.0175(+ 1(exp(*(- z 10.0) 2)))))(* 0.00005(- 1(/ z 12.0)))) 0.01) x) 0.03)))(= d/dt[y] (* 1.0(+(*(* 0.00005(- 1(/ z 12.0))) x)(*(-(* alphay(- 1(* 1.0(/ z 12.0)))) 0.0168) y))))(= d/dt[z] (* 1.0(+(*(- 12.0 z) 0.08) 0.02)))))
(assert (and 
(or ((and(= x_0_0 19)(= y_0_0 0.1)(= z_0_0 12.5)(= tau_0_0 0))))
(>= alphay_0_0 0.0502243)
(<= alphay_0_0 0.0502243)
(= [alphay_0_t tau_0_t x_0_t y_0_t z_0_t ] (integral 0.0 time_0 [alphay_0_0 tau_0_0 x_0_0 y_0_0 z_0_0 ] flow_1))
(forall_t 1 [0.0 time_0] (<= y_0_t 1))
(<=(+ x_0_t y_0_t) 4.0)
(= tau_1_0  tau_0_t)(= x_1_0  x_0_t)(= y_1_0  y_0_t)(= z_1_0  z_0_t)(= [alphay_1_t tau_1_t x_1_t y_1_t z_1_t ] (integral 0.0 time_1 [alphay_1_0 tau_1_0 x_1_0 y_1_0 z_1_0 ] flow_2))
(forall_t 2 [0.0 time_1] (<= y_1_t 1))
(or ((and(>= y_1_t 1))))))
(check-sat)
(exit)