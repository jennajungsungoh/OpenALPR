using System;
using System.Collections.Generic;

using VehicleClassifierNet;
using VehicleClassifierNet.Models;

namespace VehicleClassifierNetTest
{
    class Program
    {
        static void Main(string[] args)
        {
            // Create the object
            VehicleClassifier vehicleClassifier = new VehicleClassifier("", "", "");


            if (!vehicleClassifier.IsLoaded())
            {
                Console.WriteLine("Vehicle Classifier was not properly initialized.  Exiting");
                vehicleClassifier.Dispose();
                Console.ReadKey();
                return;
            }

            vehicleClassifier.setTopN(3);

            VehicleResponse response = vehicleClassifier.Classify( @"samples\us-3.jpg", "us", 390, 250, 830, 830);

            Console.WriteLine("Vehicle Make:");
            foreach (Candidate candidate in response.make)
            {
                Console.WriteLine("  - {0} ({1}%)", candidate.name, candidate.confidence);
            }
            Console.WriteLine("");
            Console.WriteLine("Vehicle Color:");
            foreach (Candidate candidate in response.color)
            {
                Console.WriteLine("  - {0} ({1}%)", candidate.name, candidate.confidence);
            }
            Console.WriteLine("");
            Console.WriteLine("Vehicle Make/Model:");
            foreach (Candidate candidate in response.make_model)
            {
                Console.WriteLine("  - {0} ({1}%)", candidate.name, candidate.confidence);
            }
            Console.WriteLine("");
            Console.WriteLine("Vehicle Body Type:");
            foreach (Candidate candidate in response.body_type)
            {
                Console.WriteLine("  - {0} ({1}%)", candidate.name, candidate.confidence);
            }
            Console.WriteLine("");
            Console.WriteLine("Vehicle Year:");
            foreach (Candidate candidate in response.year)
            {
                Console.WriteLine("  - {0} ({1}%)", candidate.name, candidate.confidence);
            }
            Console.WriteLine("");
            Console.WriteLine("Vehicle Orientation:");
            foreach (Candidate candidate in response.orientation)
            {
                Console.WriteLine("  - {0} ({1}%)", candidate.name, candidate.confidence);
            }

            // Free the VehicleClassifier instance's memory.
            vehicleClassifier.Dispose();

            Console.WriteLine("\nPress any key to continue...");
            Console.ReadKey();
        }
    }
}


