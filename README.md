# MAIKo+ DAQ program
This is the MAIKo+ DAQ program with Gigabit Iwaki Board.

Required VME modules
- CAEN V2495: Distributes trigger signal to all of Iwaki boards and sums busy signals.
- BeeBeans SiTPC VME controller
- REPIC RPV-132: Outputs enable signal for the Iwaki Boards and visual scaler.

useage: ./MAIKo_DAQ run_comment

Output files are saved in the directory written in "datadir.dat".

Run number automatically increments according to "runnum.dat".
