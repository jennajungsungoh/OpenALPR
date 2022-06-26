using System;
using System.Runtime.InteropServices;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace VehicleClassifierNet
{


    public class VehicleClassifier : IDisposable
    {

        private const int REALLY_BIG_PIXEL_WIDTH = 999999999;

        private string config_file;
        private string runtime_dir;
        private IntPtr native_instance;
        private bool is_initialized = false;
        

        /// <summary>
        /// VehicleClassifier library constructor. The classification types that you wish to use must each 
        /// be initialized separately with a call to LoadClassifier()
        /// </summary>
        /// <param name="config_file">The path to the openalpr.conf file.  Leave it blank to use the file in the current directory.</param>
        /// <param name="runtime_dir">The path to the runtime_data directory.  Leave it blank to use the runtime_data in the current directory</param>
        /// <param name="license_key">The OpenALPR license key.  Leave it blank to use the value from license.conf</param>
        public VehicleClassifier(string config_file = "", string runtime_dir = "", string license_key = "")
        {
            this.config_file = config_file;
            this.runtime_dir = runtime_dir;
            // Deinitialize the library

            this.is_initialized = false;
            try
            {
                // Initializes OpenALPR Vehicle Classifier on the CPU (device_type=0)
                native_instance = vehicleclassifier_init(config_file, runtime_dir, 0, 1, 0, license_key);
                this.is_initialized = true;
            }
            catch (System.DllNotFoundException)
            {
                Console.WriteLine("Could not find/load native library (libopenalpr.dll)");
            }
            
        }


        ~VehicleClassifier()
        {
            Dispose();
        }

        /// <summary>
        /// Release the memory associated with the VehicleClassifier when you are done using it
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
        }

        protected virtual void Dispose(bool disposing)
        {

            // Deinitialize the library
            if (this.is_initialized)
            {
                try
                {
                    // Destroy the native obj
                    vehicleclassifier_cleanup(native_instance);
                }
                catch (System.DllNotFoundException)
                {
                    // Ignore.  The library couldn't possibly be loaded anyway
                }
            }
            this.is_initialized = false;
        }


        /// <summary>
        /// Verifies that the Vehicle Classifier library has been initialized
        /// </summary>
        /// <returns>True if the library is loaded and ready to recognize, false otherwise.</returns>
        public bool IsLoaded()
        {
            if (!this.is_initialized)
                return false;

            try
            {
                return vehicleclassifier_is_loaded(this.native_instance) != 0;
            }
            catch (System.DllNotFoundException)
            {
                return false;
            }
        }


        /// <summary>
        /// Sets the maximum number of results for the Vehicle Classifier to return with each classification
        /// </summary>
        /// <param name="top_n">An integer describing the maximum number of results to return</param>
        public void setTopN(int top_n)
        {

            vehicleclassifier_set_topn(this.native_instance, top_n);
        }


        /// <summary>
        /// Recognizes an encoded image (e.g., JPG, PNG, etc) from a file on disk
        /// </summary>
        /// <param name="filepath">The path to the image file</param>
        /// <returns>A list of candidate classifications ordered by confidence</returns>
        /// <exception cref="InvalidOperationException">Thrown when the library has not been initialized for this classification type</exception>
        public Models.VehicleResponse Classify(string filepath, string country)
        {
            return Classify(System.IO.File.ReadAllBytes(filepath), country);
        }

        /// <summary>
        /// Recognizes an encoded image (e.g., JPG, PNG, etc) from a file on disk
        /// </summary>
        /// <param name="filepath">The path to the image file</param>
        /// <param name="x">The X offset used to create a crop before recognizing</param>
        /// <returns>A list of candidate classifications ordered by confidence</returns>
        /// <exception cref="InvalidOperationException">Thrown when the library has not been initialized for this classification type</exception>
        public Models.VehicleResponse Classify(string filepath, string country, int x, int y, int w, int h)
        {
            return Classify(System.IO.File.ReadAllBytes(filepath), country, x, y, w, h);
        }

        /// <summary>
        /// Recognizes an encoded image (e.g., JPG, PNG, etc) provided as an array of bytes
        /// </summary>
        /// <param name="image_bytes">The raw bytes that compose the image</param>
        /// <returns>A list of candidate classifications ordered by confidence</returns>
        /// <exception cref="InvalidOperationException">Thrown when the library has not been initialized for this classification type</exception>
        public Models.VehicleResponse Classify(byte[] image_bytes, string country)
        {
            return Classify(image_bytes, country, 0, 0, REALLY_BIG_PIXEL_WIDTH, REALLY_BIG_PIXEL_WIDTH);
        }
        /// <summary>
        /// Recognizes an encoded image (e.g., JPG, PNG, etc) provided as an array of bytes
        /// </summary>
        /// <param name="image_bytes">The raw bytes that compose the image</param>
        /// <returns>A list of candidate classifications ordered by confidence</returns>
        /// <exception cref="InvalidOperationException">Thrown when the library has not been initialized for this classification type</exception>
        public Models.VehicleResponse Classify( byte[] image_bytes, string country, int x, int y, int w, int h)
        {

            NativeROI roi;
            roi.x = x;
            roi.y = y;
            roi.width = w;
            roi.height = h;
            
            IntPtr unmanagedArray = Marshal.AllocHGlobal(image_bytes.Length);
            Marshal.Copy(image_bytes, 0, unmanagedArray, image_bytes.Length);

            IntPtr resp_ptr = vehicleclassifier_recognize_encodedimage(this.native_instance, country, unmanagedArray, image_bytes.Length, roi);

            Marshal.FreeHGlobal(unmanagedArray);

            // Commented out test JSON string
            //string json = @"{""version"":2,""data_type"":""alpr_results"",""epoch_time"":1476716853320,""img_width"":600,""img_height"":600,""processing_time_ms"":116.557533,""regions_of_interest"":[{""x"":0,""y"":0,""width"":600,""height"":600}],""results"":[{""plate"":""627WWI"",""confidence"":94.338623,""matches_template"":1,""plate_index"":0,""region"":""wa"",""region_confidence"":82,""processing_time_ms"":50.445648,""requested_topn"":10,""coordinates"":[{""x"":242,""y"":360},{""x"":358,""y"":362},{""x"":359,""y"":412},{""x"":241,""y"":408}],""candidates"":[{""plate"":""627WWI"",""confidence"":94.338623,""matches_template"":1},{""plate"":""627WKI"",""confidence"":80.588486,""matches_template"":1},{""plate"":""627WI"",""confidence"":79.943542,""matches_template"":0},{""plate"":""627WVI"",""confidence"":79.348930,""matches_template"":1},{""plate"":""627WRI"",""confidence"":79.196785,""matches_template"":1},{""plate"":""627WNI"",""confidence"":79.165802,""matches_template"":1}]}]}";

            string json = Marshal.PtrToStringAnsi(resp_ptr);
            Models.VehicleResponse response = JsonConvert.DeserializeObject<Models.VehicleResponse>(json);
            vehicleclassifier_free_response_string(resp_ptr);

            return response;
        }
        

        /// <summary>
        /// Recognizes an image from a .NET Bitmap object
        /// </summary>
        /// <param name="bitmap">The .NET Bitmap object to recognize</param>
        /// <returns>A list of candidate classifications ordered by confidence</returns>
        /// <exception cref="InvalidOperationException">Thrown when the library has not been initialized for this classification type</exception>
        public List<Models.Candidate> Classify(System.Drawing.Image image, string country)
        {
            return Classify(image, country, 0, 0, image.Width, image.Height);
        }

        /// <summary>
        /// Recognizes an image from a .NET Bitmap object
        /// </summary>
        /// <param name="bitmap">The .NET Bitmap object to recognize</param>
        /// <param name="x">The X offset used to create a crop before recognizing</param>
        /// <returns>A list of candidate classifications ordered by confidence</returns>
        /// <exception cref="InvalidOperationException">Thrown when the library has not been initialized for this classification type</exception>
        public List<Models.Candidate> Classify(System.Drawing.Image image, string country, int x, int y, int w, int h)
        {

            System.Drawing.Bitmap clone = new System.Drawing.Bitmap(image.Width, image.Height, System.Drawing.Imaging.PixelFormat.Format32bppRgb);
            using (System.Drawing.Graphics gr = System.Drawing.Graphics.FromImage(clone))
            {
                gr.DrawImage(image, new System.Drawing.Rectangle(0, 0, clone.Width, clone.Height));
            }

            System.Drawing.Imaging.BitmapData locked_bmp = clone.LockBits(new System.Drawing.Rectangle(0, 0, clone.Width, clone.Height),
                 System.Drawing.Imaging.ImageLockMode.ReadWrite, clone.PixelFormat);
            byte[] raw_bytes = new byte[locked_bmp.Stride * locked_bmp.Height];
            System.Runtime.InteropServices.Marshal.Copy(locked_bmp.Scan0, raw_bytes, 0, raw_bytes.Length);
            clone.UnlockBits(locked_bmp);

            int bytes_per_pixel = System.Drawing.Image.GetPixelFormatSize(clone.PixelFormat) / 8;

            NativeROI roi;
            roi.x = x;
            roi.y = y;
            roi.width = w;
            roi.height = h;

            IntPtr unmanagedArray = Marshal.AllocHGlobal(raw_bytes.Length);
            Marshal.Copy(raw_bytes, 0, unmanagedArray, raw_bytes.Length);

            IntPtr resp_ptr = vehicleclassifier_recognize_rawimage(this.native_instance, country, unmanagedArray, bytes_per_pixel, image.Width, image.Height, roi);

            Marshal.FreeHGlobal(unmanagedArray);

            string json = Marshal.PtrToStringAnsi(resp_ptr);
            List<Models.Candidate> response = JsonConvert.DeserializeObject<List<Models.Candidate>>(json);
            vehicleclassifier_free_response_string(resp_ptr);

            return response;
        }

        // Enumerate the native methods.  Handle the plumbing internally

        [StructLayout(LayoutKind.Sequential)]
        private struct NativeROI
        {
            public int x;
            public int y;
            public int width;
            public int height;
        }

        [DllImport("libopenalpr.dll")]
        private static extern IntPtr vehicleclassifier_init([MarshalAs(UnmanagedType.LPStr)] string configFile, [MarshalAs(UnmanagedType.LPStr)] string runtimeDir, 
            int device_type, int batch_size, int gpu_id, [MarshalAs(UnmanagedType.LPStr)] string licenseKey);

        [DllImport("libopenalpr.dll")]
        private static extern int vehicleclassifier_cleanup(IntPtr instance);
        
        [DllImport("libopenalpr.dll")]
        private static extern int vehicleclassifier_is_loaded(IntPtr instance);
        
        [DllImport("libopenalpr.dll")]
        private static extern IntPtr vehicleclassifier_recognize_rawimage(IntPtr instance, [MarshalAs(UnmanagedType.LPStr)] string country, IntPtr pixelData, int bytesPerPixel, int imgWidth, int imgHeight, NativeROI roi);

        [DllImport("libopenalpr.dll")]
        private static extern IntPtr vehicleclassifier_recognize_encodedimage(IntPtr instance, [MarshalAs(UnmanagedType.LPStr)] string country, IntPtr bytes, long length, NativeROI roi);

        [DllImport("libopenalpr.dll")]
        private static extern void vehicleclassifier_set_topn(IntPtr instance, int topN);

        [DllImport("libopenalpr.dll")]
        private static extern void vehicleclassifier_free_response_string(IntPtr response);
    }
}


