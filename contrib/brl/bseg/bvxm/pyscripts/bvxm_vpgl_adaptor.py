from bvxm_register import bvxm_batch, dbvalue;
#import math, numpy;
import math;

###################;
#camera loading;
###################;
def load_perspective_camera(file_path) :
  bvxm_batch.init_process("vpglLoadPerspectiveCameraProcess");
  bvxm_batch.set_input_string(0, file_path);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;
def load_affine_camera(file_path) :
  bvxm_batch.init_process("vpglLoadAffineCameraProcess");
  bvxm_batch.set_input_string(0, file_path);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;
def load_projective_camera(file_path) :
  bvxm_batch.init_process("vpglLoadProjCameraProcess");
  bvxm_batch.set_input_string(0, file_path);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;
#Scale = (scale_u, scale_v), ppoint = (u,v), center = (x,y,z), look_pt = (x,y,z), up = (x,y,z);
def create_perspective_camera( scale, ppoint, center, look_pt,up = [0,1,0] ) :
  bvxm_batch.init_process("vpglCreatePerspectiveCameraProcess");
  bvxm_batch.set_input_double(0, scale[0]);
  bvxm_batch.set_input_double(1, ppoint[0]);
  bvxm_batch.set_input_double(2, scale[1]);
  bvxm_batch.set_input_double(3, ppoint[1]);
  bvxm_batch.set_input_double(4, center[0]);
  bvxm_batch.set_input_double(5, center[1]);
  bvxm_batch.set_input_double(6, center[2]);
  bvxm_batch.set_input_double(7, look_pt[0]);
  bvxm_batch.set_input_double(8, look_pt[1]);
  bvxm_batch.set_input_double(9, look_pt[2]);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;


def create_perspective_camera_from_kml(ni, nj, right_fov, top_fov, altitude, heading, tilt, roll, cent_x, cent_y) :
  bvxm_batch.init_process("vpglCreatePerspCameraFromKMLProcess");
  bvxm_batch.set_input_unsigned(0, ni);
  bvxm_batch.set_input_unsigned(1, nj);
  bvxm_batch.set_input_double(2, right_fov);
  bvxm_batch.set_input_double(3, top_fov);
  bvxm_batch.set_input_double(4, altitude);
  bvxm_batch.set_input_double(5, heading);
  bvxm_batch.set_input_double(6, tilt);
  bvxm_batch.set_input_double(7, roll);
  bvxm_batch.set_input_double(8, cent_x);
  bvxm_batch.set_input_double(9, cent_y);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;

def load_perspective_camera_from_kml_file(NI, NJ, kml_file) :
  bvxm_batch.init_process("vpglLoadPerspCameraFromKMLFileProcess");
  bvxm_batch.set_input_unsigned(0, NI);
  bvxm_batch.set_input_unsigned(1, NJ);
  bvxm_batch.set_input_string(2, kml_file);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  (id,type) = bvxm_batch.commit_output(1);
  longitude = bvxm_batch.get_output_double(id);
  (id,type) = bvxm_batch.commit_output(2);
  latitude = bvxm_batch.get_output_double(id);
  (id,type) = bvxm_batch.commit_output(3);
  altitude = bvxm_batch.get_output_double(id);
  return cam, longitude, latitude, altitude;

#resize a camera from size0 =(ni,nj) to size1 (ni_1, nj_1);
def resample_perspective_camera( cam, size0, size1 ):
  bvxm_batch.init_process("vpglResamplePerspectiveCameraProcess");
  bvxm_batch.set_input_from_db(0, cam);
  bvxm_batch.set_input_int(1, size0[0]);
  bvxm_batch.set_input_int(2, size0[1]);
  bvxm_batch.set_input_int(3, size1[0]);
  bvxm_batch.set_input_int(4, size1[1]);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  out = dbvalue(id,type);
  return out;
#resize a camera from size0 =(ni,nj) to size1 (ni_1, nj_1);
def get_perspective_camera_center( cam):
  bvxm_batch.init_process("vpglGetPerspectiveCamCenterProcess");
  bvxm_batch.set_input_from_db(0, cam);
  bvxm_batch.run_process();
  (id, type) = bvxm_batch.commit_output(0);
  x=bvxm_batch.get_output_float(id);
  (id, type) = bvxm_batch.commit_output(1);
  y=bvxm_batch.get_output_float(id);
  (id, type) = bvxm_batch.commit_output(2);
  z=bvxm_batch.get_output_float(id);
  return x,y,z;

# returns cartesian cam center from azimuth (degrees), elevation (degrees), radius, look point;
def get_camera_center( azimuth, elevation, radius, lookPt) :
  deg_to_rad = math.pi/180.0;
  el = elevation*deg_to_rad;
  az = azimuth*deg_to_rad;
  cx = radius*math.sin(el)*math.cos(az);
  cy = radius*math.sin(el)*math.sin(az);
  cz = radius*math.cos(el);
  center = (cx + lookPt[0], cy + lookPt[1], cz + lookPt[2]);
  return center;

#returns spherical coordinates about sCenter given cartesian point;
def cart2sphere(cartPt, sCenter):
  #offset cart point;
  cartPt = numpy.subtract(cartPt, sCenter);
  rad = math.sqrt( sum(cartPt*cartPt) );
  az = math.atan2(cartPt[1],cartPt[0]);
  el = math.acos(cartPt[2]/rad);
  return (math.degrees(az), math.degrees(el), rad);

def load_rational_camera(file_path) :
  bvxm_batch.init_process("vpglLoadRationalCameraProcess");
  bvxm_batch.set_input_string(0, file_path);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;

def load_rational_camera_nitf(file_path) :
  bvxm_batch.init_process("vpglLoadRationalCameraNITFProcess");
  bvxm_batch.set_input_string(0, file_path);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;

def load_local_rational_camera(file_path) :
  bvxm_batch.init_process("vpglLoadLocalRationalCameraProcess");
  bvxm_batch.set_input_string(0, file_path);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;
def convert_local_rational_perspective_camera(local_cam) :
  bvxm_batch.init_process("vpglConvertLocalRationalToPerspectiveProcess");
  bvxm_batch.set_input_from_db(0, local_cam);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  cam = dbvalue(id,type);
  return cam;
def create_local_rational_camera(rational_cam_fname, lvcs_fname):
    bvxm_batch.init_process('vpglCreateLocalRationalCameraProcess');
    bvxm_batch.set_input_string(0,rational_cam_fname);
    bvxm_batch.set_input_string(1,lvcs_fname);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    cam = dbvalue(id,type);
    return cam;

###################;
#camera saving;
###################;
def save_rational_camera(camera,path) :
  bvxm_batch.init_process("vpglSaveRationalCameraProcess");
  bvxm_batch.set_input_from_db(0,camera);
  bvxm_batch.set_input_string(1,path);
  bvxm_batch.run_process();
def save_perspective_camera(camera,path) :
  bvxm_batch.init_process("vpglSavePerspectiveCameraProcess");
  bvxm_batch.set_input_from_db(0,camera);
  bvxm_batch.set_input_string(1,path);
  bvxm_batch.run_process();
def save_perspective_camera_vrml(camera,path) :
  bvxm_batch.init_process("vpglSavePerspectiveCameraVrmlProcess");
  bvxm_batch.set_input_from_db(0,camera);
  bvxm_batch.set_input_string(1,path);
  bvxm_batch.set_input_float(2,5.0);
  bvxm_batch.run_process();
def save_perspective_cameras_vrml(camerafolder,path) :
  bvxm_batch.init_process("vpglSavePerspectiveCamerasVrmlProcess");
  bvxm_batch.set_input_string(0,camerafolder);
  bvxm_batch.set_input_string(1,path);
  bvxm_batch.set_input_float(2,5.0);
  bvxm_batch.run_process();
#################################################;
# perspective go generic conversion;
#################################################;
def persp2gen(pcam, ni, nj, level=0) :
  bvxm_batch.init_process("vpglConvertToGenericCameraProcess");
  bvxm_batch.set_input_from_db(0, pcam);
  bvxm_batch.set_input_unsigned(1, ni);
  bvxm_batch.set_input_unsigned(2, nj);
  bvxm_batch.set_input_unsigned(3, level);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  gcam = dbvalue(id,type);
  return gcam;

def persp2genWmargin(pcam, ni, nj, margin, level=0) :
  bvxm_batch.init_process("vpglConvertToGenericCameraWithMarginProcess");
  bvxm_batch.set_input_from_db(0, pcam);
  bvxm_batch.set_input_unsigned(1, ni);
  bvxm_batch.set_input_unsigned(2, nj);
  bvxm_batch.set_input_unsigned(3, level);
  bvxm_batch.set_input_int(4, margin);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  gcam = dbvalue(id,type);
  (id,type) = bvxm_batch.commit_output(1);
  ni = bvxm_batch.get_output_unsigned(id);
  (id,type) = bvxm_batch.commit_output(2);
  nj = bvxm_batch.get_output_unsigned(id);
  (id,type) = bvxm_batch.commit_output(3);
  new_pers_cam = dbvalue(id,type);
  return (gcam, ni, nj, new_pers_cam);

#gets bounding box from a directory of cameras... (incomplete)_;
def camera_dir_planar_bbox(dir_name) :
  bvxm_batch.init_process("vpglGetBoundingBoxProcess");
  bvxm_batch.set_input_string(0, dir_name);
  bvxm_batch.run_process();

def project_point(camera,x,y,z):
    bvxm_batch.init_process('vpglProjectProcess');
    bvxm_batch.set_input_from_db(0,camera);
    bvxm_batch.set_input_float(1,x);
    bvxm_batch.set_input_float(2,y);
    bvxm_batch.set_input_float(3,z);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    u = bvxm_batch.get_output_float(id);
    (id,type) = bvxm_batch.commit_output(1);
    v = bvxm_batch.get_output_float(id);
    return (u,v);

# gets view direction at a point for a perspective camera;
def get_view_at_point(persp_cam,x,y,z):
  bvxm_batch.init_process("vpglGetViewDirectionAtPointProcess");
  bvxm_batch.set_input_from_db(0,persp_cam);
  bvxm_batch.set_input_float(1,x);
  bvxm_batch.set_input_float(2,y);
  bvxm_batch.set_input_float(3,z);
  bvxm_batch.run_process();
  (id,type) = bvxm_batch.commit_output(0);
  theta=bvxm_batch.get_output_float(id);
  (id,type) = bvxm_batch.commit_output(1);
  phi=bvxm_batch.get_output_float(id);
  return theta, phi;

def get_3d_from_depth(persp_cam,u,v,t) :
  bvxm_batch.init_process("vpglGenerate3dPointFromDepthProcess");
  bvxm_batch.set_input_from_db(0,persp_cam);
  bvxm_batch.set_input_float(1,u);
  bvxm_batch.set_input_float(2,v);
  bvxm_batch.set_input_float(3,t);
  bvxm_batch.run_process();
  (id, type) = bvxm_batch.commit_output(0);
  x=bvxm_batch.get_output_float(id);
  (id, type) = bvxm_batch.commit_output(1);
  y=bvxm_batch.get_output_float(id);
  (id, type) = bvxm_batch.commit_output(2);
  z=bvxm_batch.get_output_float(id);
  return x,y,z;

# triangulates a list of cams and a list of points;
def get_3d_from_cams( cams, points ):
    assert( len(cams) == len(points) and len(cams) > 1 );
    #list of points will just be [u1,v1,u2,v2...];
    ptlist = [];
    for p in points:
        ptlist.append(p[0]);
        ptlist.append(p[1]);
    #list of cam ids (type will be checked in C++);
    camlist = [];
    for cam in cams:
        camlist.append(cam.id);
    bvxm_batch.init_process("vpglGenerate3dPointFromCamsProcess");
    bvxm_batch.set_input_unsigned_array(0, camlist);
    bvxm_batch.set_input_int_array(1, ptlist);
    bvxm_batch.run_process();
    (id, type) = bvxm_batch.commit_output(0);
    x=bvxm_batch.get_output_float(id);
    (id, type) = bvxm_batch.commit_output(1);
    y=bvxm_batch.get_output_float(id);
    (id, type) = bvxm_batch.commit_output(2);
    z=bvxm_batch.get_output_float(id);
    return x,y,z;

# create a generic camera;
def convert_to_generic_camera(cam_in, ni, nj, level=0):
    bvxm_batch.init_process('vpglConvertToGenericCameraProcess');
    bvxm_batch.set_input_from_db(0,cam_in);
    bvxm_batch.set_input_unsigned(1,ni);
    bvxm_batch.set_input_unsigned(2,nj);
    bvxm_batch.set_input_unsigned(3,level);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    generic_cam = dbvalue(id,type);
    return generic_cam;

# create a generic camera from a local rational with user-specified z range;
def convert_local_rational_to_generic(cam_in, ni, nj, min_z, max_z, level=0):
    bvxm_batch.init_process('vpglConvertLocalRationalToGenericProcess');
    bvxm_batch.set_input_from_db(0,cam_in);
    bvxm_batch.set_input_unsigned(1,ni);
    bvxm_batch.set_input_unsigned(2,nj);
    bvxm_batch.set_input_float(3,min_z);
    bvxm_batch.set_input_float(4,max_z);
    bvxm_batch.set_input_unsigned(5,level);
    if not bvxm_batch.run_process():
        return None;
    (id,type) = bvxm_batch.commit_output(0);
    generic_cam = dbvalue(id,type);
    return generic_cam;

# correct a rational camera;
def correct_rational_camera(cam_in, offset_x, offset_y):
    bvxm_batch.init_process('vpglCorrectRationalCameraProcess');
    bvxm_batch.set_input_from_db(0,cam_in);
    bvxm_batch.set_input_double(1,offset_x);
    bvxm_batch.set_input_double(2,offset_y);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    corrected_cam = dbvalue(id,type);
    return corrected_cam;

def get_rational_camera_offsets(cam_in):
    bvxm_batch.init_process('vpglGetRationalCameraOffsetsProcess');
    bvxm_batch.set_input_from_db(0,cam_in);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    offset_u = bvxm_batch.get_output_double(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(1);
    offset_v = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    return (offset_u,offset_v);

# convert lat,lon,el to local coordinates;
def convert_to_local_coordinates(lvcs_filename,lat,lon,el):
    bvxm_batch.init_process('vpglConvertToLocalCoordinatesProcess');
    bvxm_batch.set_input_string(0,lvcs_filename);
    bvxm_batch.set_input_float(1,lat);
    bvxm_batch.set_input_float(2,lon);
    bvxm_batch.set_input_float(3,el);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    x = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(1);
    y = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(2);
    z = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    return (x,y,z);

# convert lat,lon,el to local coordinates;
def convert_to_local_coordinates2(lvcs,lat,lon,el):
    bvxm_batch.init_process('vpglConvertToLocalCoordinatesProcess2');
    bvxm_batch.set_input_from_db(0,lvcs);
    bvxm_batch.set_input_float(1,lat);
    bvxm_batch.set_input_float(2,lon);
    bvxm_batch.set_input_float(3,el);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    x = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(1);
    y = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(2);
    z = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    return (x,y,z);
# convert lat,lon,el to local coordinates;
def convert_local_to_global_coordinates(lvcs,x,y,z):
    bvxm_batch.init_process('vpglConvertLocalToGlobalCoordinatesProcess');
    bvxm_batch.set_input_from_db(0,lvcs);
    bvxm_batch.set_input_float(1,x);
    bvxm_batch.set_input_float(2,y);
    bvxm_batch.set_input_float(3,z);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    lat = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(1);
    lon = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    (id,type) = bvxm_batch.commit_output(2);
    el = bvxm_batch.get_output_float(id);
    bvxm_batch.remove_data(id);
    return (lat,lon,el);
# convert lat,lon,el to local coordinates;
def create_lvcs(lat,lon,el,csname):
    bvxm_batch.init_process('vpglCreateLVCSProcess');
    bvxm_batch.set_input_float(0,lat);
    bvxm_batch.set_input_float(1,lon);
    bvxm_batch.set_input_float(2,el);
    bvxm_batch.set_input_string(3,csname);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    lvcs = dbvalue(id,type);
    return lvcs;

# randomly sample a camera rotated around principle axis;
def perturb_camera(cam_in, angle, rng):
    bvxm_batch.init_process('vpglPerturbPerspCamOrientProcess');
    bvxm_batch.set_input_from_db(0,cam_in);
    bvxm_batch.set_input_float(1,angle);
    bvxm_batch.set_input_from_db(2,rng);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    pert_cam = dbvalue(id,type);
    (theta_id,type) = bvxm_batch.commit_output(1);
    (phi_id,type) = bvxm_batch.commit_output(2);
    theta = bvxm_batch.get_output_float(theta_id);
    phi = bvxm_batch.get_output_float(phi_id);
    return pert_cam, theta, phi;

def write_perspective_cam_vrml(vrml_filename, pcam, camera_rad, axis_length, r, g, b):
    bvxm_batch.init_process("bvrmlWritePerspectiveCamProcess");
    bvxm_batch.set_input_string(0,vrml_filename);
    bvxm_batch.set_input_from_db(1,pcam);
    bvxm_batch.set_input_float(2,camera_rad);
    bvxm_batch.set_input_float(3,axis_length);
    bvxm_batch.set_input_float(4,r);
    bvxm_batch.set_input_float(5,g);
    bvxm_batch.set_input_float(6,b);
    bvxm_batch.run_process();

# rotate a camera around principle axis;
def rotate_perspective_camera(cam_in, theta, phi):
    bvxm_batch.init_process('vpglRotatePerspCamProcess');
    bvxm_batch.set_input_from_db(0,cam_in);
    bvxm_batch.set_input_float(1,theta);
    bvxm_batch.set_input_float(2,phi);
    bvxm_batch.run_process();
    (id,type) = bvxm_batch.commit_output(0);
    rot_cam = dbvalue(id,type);
    return rot_cam;

def get_perspective_cam_center(pcam):
    bvxm_batch.init_process("vpglGetPerspectiveCamCenterProcess");
    bvxm_batch.set_input_from_db(0, pcam);
    bvxm_batch.run_process();
    (x_id,x_type) = bvxm_batch.commit_output(0);
    x = bvxm_batch.get_output_float(x_id);
    (y_id,type) = bvxm_batch.commit_output(1);
    y = bvxm_batch.get_output_float(y_id);
    (z_id,type) = bvxm_batch.commit_output(2);
    z = bvxm_batch.get_output_float(z_id);
    return x, y, z;

def create_perspective_camera2(pcam, cent_x, cent_y, cent_z):
    bvxm_batch.init_process("vpglCreatePerspectiveCameraProcess2");
    bvxm_batch.set_input_from_db(0, pcam);
    bvxm_batch.set_input_float(1, cent_x);
    bvxm_batch.set_input_float(2, cent_y);
    bvxm_batch.set_input_float(3, cent_z);
    bvxm_batch.run_process();
    (c_id,c_type) = bvxm_batch.commit_output(0);
    cam = dbvalue(c_id,c_type);
    return cam;

def create_perspective_camera_with_rot(pcam, phi, theta, cent_x, cent_y, cent_z):
    bvxm_batch.init_process("vpglCreatePerspectiveCameraProcess3");
    bvxm_batch.set_input_from_db(0, pcam);
    bvxm_batch.set_input_float(1, phi);
    bvxm_batch.set_input_float(2, theta);
    bvxm_batch.set_input_float(3, cent_x);
    bvxm_batch.set_input_float(4, cent_y);
    bvxm_batch.set_input_float(5, cent_z);
    bvxm_batch.run_process();
    (c_id,c_type) = bvxm_batch.commit_output(0);
    cam = dbvalue(c_id,c_type);
    return cam;

def get_nitf_footprint(nitf_list_filename, out_kml_filename):
    bvxm_batch.init_process('vpglNITFFootprintProcess');
    bvxm_batch.set_input_string(0,nitf_list_filename);
    bvxm_batch.set_input_string(1,out_kml_filename);
    bvxm_batch.run_process();

def get_geocam_footprint(geocam, geotiff_filename, out_kml_filename,init_finish=True):
    bvxm_batch.init_process('vpglGeoFootprintProcess');
    bvxm_batch.set_input_from_db(0,geocam);
    bvxm_batch.set_input_string(1,geotiff_filename);
    bvxm_batch.set_input_string(2,out_kml_filename);
    bvxm_batch.set_input_bool(3,init_finish);
    bvxm_batch.run_process();

def load_geotiff_cam(tfw_filename, lvcs=0, utm_zone=0, utm_hemisphere=0):
    bvxm_batch.init_process("vpglLoadGeoCameraProcess");
    bvxm_batch.set_input_string(0, tfw_filename);
    if lvcs != 0:
      bvxm_batch.set_input_from_db(1, lvcs);
    bvxm_batch.set_input_int(2, utm_zone);
    bvxm_batch.set_input_unsigned(3, utm_hemisphere);
    bvxm_batch.run_process();
    (c_id,c_type) = bvxm_batch.commit_output(0);
    cam = dbvalue(c_id,c_type);
    return cam;

def save_geocam_to_tfw(cam, tfw_filename):
    bvxm_batch.init_process("vpglSaveGeoCameraTFWProcess");
    bvxm_batch.set_input_from_db(0, cam);
    bvxm_batch.set_input_string(1, tfw_filename);
    bvxm_batch.run_process();

def load_geotiff_cam2(filename, ni, nj):
    bvxm_batch.init_process("vpglLoadGeoCameraProcess2");
    bvxm_batch.set_input_string(0, filename);
    bvxm_batch.set_input_unsigned(1, ni);
    bvxm_batch.set_input_unsigned(2, nj);
    bvxm_batch.run_process();
    (c_id,c_type) = bvxm_batch.commit_output(0);
    cam = dbvalue(c_id,c_type);
    return cam;

def translate_geo_camera(geocam, x, y):
    bvxm_batch.init_process("vpglTranslateGeoCameraProcess");
    bvxm_batch.set_input_from_db(0, geocam);
    bvxm_batch.set_input_double(1, x);
    bvxm_batch.set_input_double(2, y);
    bvxm_batch.run_process();
    (c_id, c_type) = bvxm_batch.commit_output(0);
    cam = dbvalue(c_id, c_type);
    return cam;

def geo2generic(geocam, ni, nj, scene_height, level):
    bvxm_batch.init_process("vpglConvertGeoCameraToGenericProcess");
    bvxm_batch.set_input_from_db(0, geocam);
    bvxm_batch.set_input_int(1, ni);
    bvxm_batch.set_input_int(2, nj);
    bvxm_batch.set_input_double(3, scene_height);
    bvxm_batch.set_input_int(4, level);
    bvxm_batch.run_process();
    (c_id, c_type) = bvxm_batch.commit_output(0);
    cam = dbvalue(c_id, c_type);
    return cam;

def save_lvcs(lat,lon,hae,lvcs_filename):
    bvxm_batch.init_process("vpglSaveLVCSProcess");
    bvxm_batch.set_input_float(0,lat);
    bvxm_batch.set_input_float(1,lon);
    bvxm_batch.set_input_float(2,hae);
    bvxm_batch.set_input_string(3,lvcs_filename);
    bvxm_batch.run_process();
    return;

def geo_cam_global_to_img(geocam, lon, lat):
    bvxm_batch.init_process("vpglGeoGlobalToImgProcess");
    bvxm_batch.set_input_from_db(0, geocam);
    bvxm_batch.set_input_double(1, lon);
    bvxm_batch.set_input_double(2, lat);
    bvxm_batch.run_process();
    (id, type) = bvxm_batch.commit_output(0);
    u = bvxm_batch.get_output_int(id);
    (id, type) = bvxm_batch.commit_output(1);
    v = bvxm_batch.get_output_int(id);
    return u, v;

def convert_perspective_to_nvm(cams_dir,imgs_dir, output_nvm):
    bvxm_batch.init_process("vpglExportCamerasToNvmProcess");
    bvxm_batch.set_input_string(0, cams_dir);
    bvxm_batch.set_input_string(1, imgs_dir);
    bvxm_batch.set_input_string(2, output_nvm);
    return bvxm_batch.run_process();
