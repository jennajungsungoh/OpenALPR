/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Linq;
using System.Reflection;
using System.Windows.Forms;
using AlprNet;
using AlprNet.Models;

namespace AlprNetGuiTest
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        public static string AssemblyDirectory
        {
            get
            {
                var codeBase = Assembly.GetExecutingAssembly().CodeBase;
                var uri = new UriBuilder(codeBase);
                var path = Uri.UnescapeDataString(uri.Path);
                return Path.GetDirectoryName(path);
            }
        }

        public Rectangle boundingRectangle(List<Point> points)
        {
            // Add checks here, if necessary, to make sure that points is not null,
            // and that it contains at least one (or perhaps two?) elements

            var minX = points.Min(p => p.X);
            var minY = points.Min(p => p.Y);
            var maxX = points.Max(p => p.X);
            var maxY = points.Max(p => p.Y);

            return new Rectangle(new Point(minX, minY), new Size(maxX - minX, maxY - minY));
        }

        private static Image cropImage(Image img, Rectangle cropArea)
        {
            const int MAX_WIDTH = 200;
            var bmpImage = new Bitmap(img);
            Bitmap bmp = bmpImage.Clone(cropArea, bmpImage.PixelFormat);
            if (bmp.Size.Width > MAX_WIDTH)
                return new Bitmap(bmp, new Size(MAX_WIDTH, bmp.Size.Height / (bmp.Size.Width / MAX_WIDTH)));

            return bmp;
        }

        public static Bitmap combineImages(List<Image> images)
        {
            //read all images into memory
            Bitmap finalImage = null;

            try
            {
                var width = 0;
                var height = 0;

                foreach (var bmp in images)
                {
                    width += bmp.Width;
                    height = bmp.Height > height ? bmp.Height : height;
                }

                //create a bitmap to hold the combined image
                finalImage = new Bitmap(width, height);

                //get a graphics object from the image so we can draw on it
                using (var g = Graphics.FromImage(finalImage))
                {
                    //set background color
                    g.Clear(Color.Black);

                    //go through each image and draw it on the final image
                    var offset = 0;
                    foreach (Bitmap image in images)
                    {
                        g.DrawImage(image,
                                    new Rectangle(offset, 0, image.Width, image.Height));
                        offset += image.Width;
                    }
                }

                return finalImage;
            }
            catch (Exception ex)
            {
                if (finalImage != null)
                    finalImage.Dispose();

                throw ex;
            }
            finally
            {
                //clean up memory
                foreach (var image in images)
                {
                    image.Dispose();
                }
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (openFileDialog.ShowDialog(this) == DialogResult.OK)
            {
                processImageFile(openFileDialog.FileName);
            }
        }

        
        private void processImageFile(string fileName)
        {
            resetControls();
            var region = (string) comboRegion.SelectedItem;
            String config_file =  Path.Combine(AssemblyDirectory, "openalpr.conf");
            String runtime_data_dir = Path.Combine(AssemblyDirectory, "runtime_data");


            using (var alpr = new Alpr(region, config_file, runtime_data_dir))
            {
                alpr.Initialize();

                if (!alpr.IsLoaded())
                {
                    lbxPlates.Items.Add("Error initializing OpenALPR");
                    return;
                }
                picOriginal.ImageLocation = fileName;
                picOriginal.Load();

                var results = alpr.Recognize(fileName);

                var images = new List<Image>(results.results.Count());
                var i = 1;
                foreach (var result in results.results)
                {
                    List<Point> points = new List<Point>();
                    foreach (Coordinate c in result.coordinates)
                        points.Add(new Point(c.x, c.y));

                    var rect = boundingRectangle(points);
                    var img = Image.FromFile(fileName);
                    var cropped = cropImage(img, rect);
                    images.Add(cropped);


                    lbxPlates.Items.Add("\t\t-- Plate #" + i++ + " --");
                    foreach (var plate in result.candidates)
                    {

                        lbxPlates.Items.Add(string.Format(@"{0} {1}% {2}",
                                                          plate.plate.PadRight(12),
                                                          plate.confidence.ToString("N1").PadLeft(8),
                                                          plate.matches_template.ToString().PadLeft(8)));
                    }
                }

                if (images.Any())
                {
                    picLicensePlate.Image = combineImages(images);
                }
            }
        }

        private void resetControls()
        {
            picOriginal.Image = null;
            picLicensePlate.Image = null;
            lbxPlates.Items.Clear();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            resetControls();

            comboRegion.DropDownStyle = ComboBoxStyle.DropDownList;

            String runtime_data_dir = Path.Combine(AssemblyDirectory, "runtime_data");
            String config_dir = Path.Combine(runtime_data_dir, "config");
            string[] conf_files = Directory.GetFiles(config_dir, "*.conf", SearchOption.TopDirectoryOnly);
            foreach (string conf in conf_files)
            {
                string confname = Path.GetFileNameWithoutExtension(conf);
                comboRegion.Items.Add(confname);
            }
            comboRegion.SelectedItem = "us";
        }
    }
}