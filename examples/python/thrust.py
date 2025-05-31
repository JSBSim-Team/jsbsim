import matplotlib.pyplot as plt
import numpy as np

rated_thrust = 60690 - 60690*0.05

k_norm = 492.7340552

density = 0.0024192

n1norm = []
thrust = []
jsbsim_thrust = []

for n1 in np.arange(0, 1.01, 0.01):
    thrust.append(k_norm * rated_thrust * n1**1.67 * density + 60690*0.05)
    jsbsim_thrust.append(n1**2 * rated_thrust + 60690*0.05)
    n1norm.append(n1 * (117.5 - 23.7) + 23.7)

plt.figure()
plt.plot(n1norm, thrust, label='$T = k \\rho N1_{norm}^{1.67}$')
plt.plot(n1norm, jsbsim_thrust, label='JSBSim')
plt.scatter([108.1], [60690], label='Ground truth point', color='red')
plt.axhline(y=rated_thrust + 60690*0.05, linestyle='-.')
plt.xlabel('N1 %')
plt.ylabel('Thrust (lbf)')
plt.title('Static Thrust vs N1 for Sea-level OAT = 10C')
plt.legend()
plt.show()

