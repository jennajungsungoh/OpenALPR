using System;
using System.Collections.Generic;
using System.Text;

namespace VehicleClassifierNet.Models
{
    public class VehicleResponse
    {
        public IList<Candidate> color { get; set; }
        public IList<Candidate> make { get; set; }
        public IList<Candidate> make_model { get; set; }
        public IList<Candidate> body_type { get; set; }
        public IList<Candidate> year { get; set; }
        public IList<Candidate> orientation { get; set; }
    }
}
