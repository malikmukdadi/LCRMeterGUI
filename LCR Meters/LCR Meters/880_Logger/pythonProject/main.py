import scpi880
import time
import xlsxwriter

instrument = scpi880.connect("COM3", post_command_delay=600)  # Windows users can just pass in 'COMx'

instrument.set_frequency(100)  # Sets the frequency for measurement
print("Frequency set to: 100 Hz")
instrument.set_equiv_parallel()  # Sets measurement mode to parallel
print("Measurement mode set to: Parallel")
instrument.set_primary("C")  # Sets primary measurement to Capacitance
print("Primary reading set to: Capacitance")
instrument.set_secondary("D")  # Sets secondary measurement to equivalent series resistance
print("Secondary reading set to: Dissipation")

filename = 'LCR880_TestRun1.xlsx'  # Enter filename here

row1 = 1 # Rows are for each rotation for measurements, in this case between C/D -> Z/THETA -> ESR/DSR
row2 = 1 # Z/THETA
row3 = 1 # ESR/DSR
workbook = xlsxwriter.Workbook(filename)
worksheet = workbook.add_worksheet()
worksheet.write(0, 0, "Date") # Setting the title of each column
worksheet.write(0, 1, "Time")
worksheet.write(0, 2, "Capacitance")
worksheet.write(0, 3, "Dissipation")
worksheet.write(0, 4, "Tolerance")

worksheet.write(0, 6, "Time")
worksheet.write(0, 7, "Impedance")
worksheet.write(0, 8, "Phase Angle")
worksheet.write(0, 9, "Tolerance")

worksheet.write(0, 11, "Time")
worksheet.write(0, 12, "Direct Current Resistance")
worksheet.write(0, 13, "Equivalent Series Resistance")
worksheet.write(0, 14, "Tolerance")

try:
    while True:
        startTime = time.time() # Start time used to measure total length of test
        firstInterval = time.time() + 10 # First measurement set to 5 minutes
        while time.time() < firstInterval: # Loop for 5 minutes
            temp = instrument.fetch() # Get measurement from LCR Meter
            print(temp) # Print it to console
            worksheet.write(row1, 0, time.asctime()) # Write out date to respective columns
            worksheet.write(row1, 1, time.time() - startTime)
            worksheet.write(row1, 2, temp[0])
            worksheet.write(row1, 3, temp[1])
            worksheet.write(row1, 4, temp[2])
            row1 += 1  # increment row for the next loop

        instrument.set_primary("Z") # Sets primary to impedance
        print("Primary reading set to: Impedance")
        instrument.set_secondary("THETA") # Sets secondary to phase angle
        print("Secondary reading set to: Phase Angle")

        secondInterval = time.time() + 10 # Second measurement set for 30 seconds
        while time.time() < secondInterval: # Same deal as the first loop
            temp = instrument.fetch()
            print(temp)
            worksheet.write(row2, 6, time.time() - startTime)
            worksheet.write(row2, 7, temp[0])
            worksheet.write(row2, 8, temp[1])
            worksheet.write(row2, 9, temp[2])
            row2 += 1

        instrument.set_primary("DCR") # Sets primary to DCR
        print("Primary reading set to: Direct Current Resistance")
        # instrument.set_secondary("ESR") # Sets secondary to ESR
        print("Secondary reading set to: Equivalent Series Resistance")
        thirdInterval = time.time() + 10  # Third measurement set for 30 seconds
        while time.time() < thirdInterval:  # Same deal as the first loop
            temp = instrument.fetch()
            print(temp)
            worksheet.write(row1, 11, time.time() - startTime)
            worksheet.write(row3, 12, temp[0])
            worksheet.write(row3, 13, temp[1])
            row3 += 1

        instrument.set_primary("C")  # Sets primary measurement back to Capacitance
        print("Primary reading set to: Capacitance")
        instrument.set_secondary("D")  # Sets secondary measurement back to equivalent series resistance
        print("Secondary reading set to: Dissipation factor")

except KeyboardInterrupt:  # loop forever until you press ctrl + c which is a keyboard interrupt
    workbook.close()  # Program completes and then closes/saves excel workbook
