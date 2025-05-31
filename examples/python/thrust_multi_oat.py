import matplotlib.pyplot as plt
import numpy as np

rated_thrust = 60690 - 60690*0.05

k_norm = 492.7340552

density = 0.0024192

n1norm = []
thrust = []
jsbsim_thrust = []

plt.figure()


for data in [('0C', 0.0025077), ('10C', 0.0024192), ('20C', 0.0023366), ('30C', 0.0022596), ('40C', 0.0021874), ('50C', 0.0021197), ('60C', 0.0020561)]:
    temp, density = data
    for n1 in np.arange(0, 1.01, 0.01):
        thrust.append(k_norm * rated_thrust * n1**1.67 * density + 60690*0.05)
        jsbsim_thrust.append(n1**2 * rated_thrust + 60690*0.05)
        n1norm.append(n1 * (117.5 - 23.7) + 23.7)

    plt.plot(n1norm, thrust, label=temp)
    n1norm.clear()
    thrust.clear()

plt.scatter([106.3, 108.1, 109.9, 111.7], [60690, 60690, 60690, 60690], label='Ground truth points', color='red')
plt.scatter([109.8, 108, 106.2], [53858+60690*0.05, 50382+60690*0.05, 47140+60690*0.05], label='Post OAT corner points', color='green')
plt.axhline(y=rated_thrust + 60690*0.05, linestyle='-.')
plt.xlabel('N1 %')
plt.ylabel('Thrust (lbf)')
plt.title('Static Thrust vs N1 for Sea-level Varying OAT')
plt.legend()
plt.show()

