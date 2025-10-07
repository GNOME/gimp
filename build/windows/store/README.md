# GIMP MSIX HowTo

## Stable and Development releases

The MS Store/Partner Center hosts our stable and development point releases:
https://partner.microsoft.com/dashboard/apps-and-games/overview

Base rule to update the "GIMP" or "GIMP (Preview)" entry:

* Regularly, a .msixupload will be generated at each tagged commit. It also can be
  with a custom pipeline valuating `GIMP_CI_MS_STORE` variable to `${REVISION}` as
  briefly explained in ["Versioning the MSIX"](#versioning-the-msix) section.
  In the process, it will be auto submitted (without changelog) to Partner Center.
  (Any case, **please double-check on Partner Center** if everything is done.)

## Maintaining the MSIX

If the submission from CI does not work, check on MS Entra Admin Center for the
client secret of 'GITLAB_CI' app. If expired, create a new one and add it on
GitLab as a masked, hidden and protected variable. Then, retrigger the Store job.
That is usually faster than doing a manual submission as per instructions below:

1. The link above will open 'Partner Center', the frontend to submit .msixupload

2. With your Microsoft Entra ID account (@*onmicrosoft.com), log-in as customary

3. Clicking on the "Start update" button will make possible to change some things.
   Only 'Packages' and 'Store listings' sections are needed. On 'Packages' you will
   add the generated .msixupload and on 'Store listings' the brief changelog.

If the .msix* starts to be refused to certification or to self-signing,
run `build\windows\store\3_dist-gimp-winsdk.ps1 WACK` locally to see if it
still complies with the latest Windows policies. Make sure to update WinSDK.

## Versioning the MSIX

* Every new .msixupload submission (with different content) needs a bumped version.
  But MS Store reserves revisions (the fourth digit after last dot) for its own use.
  https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/app-package-requirements
  So, our MSIX have a special micro versioning to we have revisions. Check the .ps1.
