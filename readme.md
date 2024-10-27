Compile & run
```powershell
gcc challange.c
./a
```
To see the PLL output in console, uncomment printf
```c
    for (int i = 0; i < DATA_LENGTH * CYCLE; i++)
    {   
        estimateFrequencyAndTheta(&ddata, DATA_LENGTH*CYCLE, i);
        //printf("Estimated frequency: %.2f[Hz], angle: %.2f[deg]\n",ddata.F_est/2/PI,ddata.Theta_est*180/PI);		
    }
```